#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QScopedPointer>

#include "DataExporter.h"
#include "PublishSpriteSheet.h"
#include "SpriteAtlas.h"
#include "SpritePackerProjectFile.h"

namespace {

struct PackOptions {
    QString trimMode = "Rect";
    QString algorithm = "Rect";
    int trim = 1;
    float epsilon = 5.0f;
    int textureBorder = 0;
    int spriteBorder = 2;
    bool heuristicMask = false;
    bool rotateSprites = false;
    bool powerOfTwo = false;
    bool forceSquared = false;
    int maxSize = 8192;
    float scale = 1.0f;
    QString dataFormat = "cocos2d";
    ImageFormat imageFormat = kPNG;
    PixelFormat pixelFormat = kARGB8888;
    bool premultiplied = true;
    bool trimSpriteNames = false;
    bool prependSmartFolderName = false;
    int pngCompression = 9;
    int webpQuality = 80;
    int jpgQuality = 80;
    bool pngQuant = false;
    QString pngQuantQuality = "80-95";
};

void printError(const QString& message)
{
    qCritical().noquote() << "error:" << message;
}

QScopedPointer<SpritePackerProjectFile> openProject(const QFileInfo& source)
{
    const QString suffix = source.suffix().toLower();
    if (suffix == "json" || suffix == "ssp") {
        return QScopedPointer<SpritePackerProjectFile>(new SpritePackerProjectFile);
    }
    if (suffix == "tps") {
        return QScopedPointer<SpritePackerProjectFile>(new SpritePackerProjectFileTPS);
    }
    return QScopedPointer<SpritePackerProjectFile>(nullptr);
}

bool parseInteger(const QCommandLineParser& parser,
                  const QString& option,
                  int minimum,
                  int maximum,
                  int* value,
                  QString* error)
{
    if (!parser.isSet(option)) return true;
    bool ok = false;
    const int parsed = parser.value(option).toInt(&ok);
    if (!ok || parsed < minimum || parsed > maximum) {
        *error = QString("--%1 must be an integer from %2 to %3.")
            .arg(option).arg(minimum).arg(maximum);
        return false;
    }
    *value = parsed;
    return true;
}

bool parseFloat(const QCommandLineParser& parser,
                const QString& option,
                float minimumExclusive,
                float* value,
                QString* error)
{
    if (!parser.isSet(option)) return true;
    bool ok = false;
    const float parsed = parser.value(option).toFloat(&ok);
    if (!ok || parsed <= minimumExclusive) {
        *error = QString("--%1 must be greater than %2.").arg(option).arg(minimumExclusive);
        return false;
    }
    *value = parsed;
    return true;
}

QString normalizedMode(const QString& value)
{
    if (value.compare("rect", Qt::CaseInsensitive) == 0) return "Rect";
    if (value.compare("polygon", Qt::CaseInsensitive) == 0) return "Polygon";
    return {};
}

QString textureFormatName(ImageFormat format)
{
    switch (format) {
        case kWEBP: return "webp";
        case kJPG: return "jpg";
        default: return "png";
    }
}

ImageFormat textureFormatFromName(const QString& value)
{
    if (value.compare("png", Qt::CaseInsensitive) == 0) return kPNG;
    if (value.compare("webp", Qt::CaseInsensitive) == 0) return kWEBP;
    if (value.compare("jpg", Qt::CaseInsensitive) == 0
        || value.compare("jpeg", Qt::CaseInsensitive) == 0) {
        return kJPG;
    }
    return static_cast<ImageFormat>(-1);
}

QByteArray textureWriterFormat(ImageFormat format)
{
    switch (format) {
        case kWEBP: return "webp";
        case kJPG: return "jpeg";
        default: return "png";
    }
}

PackOptions optionsFromProject(const SpritePackerProjectFile& project)
{
    PackOptions options;
    options.trimMode = normalizedMode(project.trimMode());
    options.algorithm = normalizedMode(project.algorithm());
    options.trim = project.trimThreshold();
    options.epsilon = project.epsilon();
    options.textureBorder = project.textureBorder();
    options.spriteBorder = project.spriteBorder();
    options.heuristicMask = project.heuristicMask();
    options.rotateSprites = project.rotateSprites();
    options.dataFormat = project.dataFormat().isEmpty() ? "cocos2d" : project.dataFormat().toLower();
    options.imageFormat = project.imageFormat();
    options.pixelFormat = project.pixelFormat();
    options.premultiplied = project.premultiplied();
    options.trimSpriteNames = project.trimSpriteNames();
    options.prependSmartFolderName = project.prependSmartFolderName();
    options.pngQuant = project.pngOptMode().compare("Lossy", Qt::CaseInsensitive) == 0
                    || project.pngOptMode().compare("PngQuant", Qt::CaseInsensitive) == 0;
    options.pngQuantQuality = project.pngQuantQuality();
    options.webpQuality = project.webpQuality();
    options.jpgQuality = project.jpgQuality();
    return options;
}

bool normalizePngQuantQuality(const QString& value, QString* normalized, QString* error)
{
    static const QRegularExpression pattern("^(\\d{1,3})-(\\d{1,3})$");
    const QRegularExpressionMatch match = pattern.match(value.trimmed());
    if (!match.hasMatch()) {
        *error = "--pngquant-quality must use min-max syntax, for example 80-95.";
        return false;
    }
    const int minimum = match.captured(1).toInt();
    const int maximum = match.captured(2).toInt();
    if (minimum < 0 || maximum > 100 || minimum > maximum) {
        *error = "--pngquant-quality values must satisfy 0 <= min <= max <= 100.";
        return false;
    }
    *normalized = QString::number(minimum) + "-" + QString::number(maximum);
    return true;
}

bool applyOverrides(const QCommandLineParser& parser, PackOptions* options, QString* error)
{
    if (options->trimMode.isEmpty()) {
        *error = "Project trim mode must be Rect or Polygon.";
        return false;
    }
    if (options->algorithm.isEmpty()) {
        *error = "Project packing algorithm must be Rect or Polygon.";
        return false;
    }
    if (parser.isSet("trim-mode")) {
        options->trimMode = normalizedMode(parser.value("trim-mode"));
        if (options->trimMode.isEmpty()) {
            *error = "--trim-mode must be Rect or Polygon.";
            return false;
        }
    }
    if (parser.isSet("algorithm")) {
        options->algorithm = normalizedMode(parser.value("algorithm"));
        if (options->algorithm.isEmpty()) {
            *error = "--algorithm must be Rect or Polygon.";
            return false;
        }
    }
    if (!parseInteger(parser, "trim", 0, 255, &options->trim, error)
        || !parseInteger(parser, "texture-border", 0, 4096, &options->textureBorder, error)
        || !parseInteger(parser, "sprite-border", 0, 4096, &options->spriteBorder, error)
        || !parseInteger(parser, "max-size", 1, 65536, &options->maxSize, error)
        || !parseInteger(parser, "png-compression", 0, 9, &options->pngCompression, error)
        || !parseInteger(parser, "webp-quality", 0, 100, &options->webpQuality, error)
        || !parseInteger(parser, "jpg-quality", 0, 100, &options->jpgQuality, error)
        || !parseFloat(parser, "epsilon", 0.0f, &options->epsilon, error)
        || !parseFloat(parser, "scale", 0.0f, &options->scale, error)) {
        return false;
    }

    if (parser.isSet("power-of-two")) options->powerOfTwo = true;
    if (parser.isSet("force-squared")) options->forceSquared = true;
    if (parser.isSet("heuristic-mask")) options->heuristicMask = true;
    if (parser.isSet("rotate")) options->rotateSprites = true;
    if (parser.isSet("trim-sprite-names")) options->trimSpriteNames = true;
    if (parser.isSet("prepend-smart-folder-name")) options->prependSmartFolderName = true;
    if (parser.isSet("no-premultiplied")) options->premultiplied = false;
    if (parser.isSet("texture-format")) {
        options->imageFormat = textureFormatFromName(parser.value("texture-format"));
    }
    if (options->imageFormat != kPNG
        && options->imageFormat != kWEBP
        && options->imageFormat != kJPG) {
        *error = "Texture format must be png, webp, or jpg.";
        return false;
    }
    const QByteArray requiredWriter = textureWriterFormat(options->imageFormat);
    if (!QImageWriter::supportedImageFormats().contains(requiredWriter)) {
        *error = QString("Qt image writer '%1' is unavailable. Install the corresponding Qt image plugin.")
            .arg(QString::fromLatin1(requiredWriter));
        return false;
    }
    if (parser.isSet("pngquant") && parser.isSet("no-pngquant")) {
        *error = "--pngquant and --no-pngquant cannot be used together.";
        return false;
    }
    if (parser.isSet("pngquant")) options->pngQuant = true;
    if (parser.isSet("no-pngquant")) options->pngQuant = false;
    if (parser.isSet("pngquant-quality")) {
        if (parser.isSet("no-pngquant")) {
            *error = "--pngquant-quality cannot be combined with --no-pngquant.";
            return false;
        }
        if (!normalizePngQuantQuality(parser.value("pngquant-quality"),
                                     &options->pngQuantQuality,
                                     error)) {
            return false;
        }
        options->pngQuant = true;
    } else if (options->pngQuant
               && !normalizePngQuantQuality(options->pngQuantQuality,
                                             &options->pngQuantQuality,
                                             error)) {
        *error = "Project pngQuantQuality must use min-max values from 0 to 100.";
        return false;
    }

    if (parser.isSet("format")) options->dataFormat = parser.value("format").toLower();
    QStringList formats = DataExporter::supportedFormats();
    formats.append("none");
    if (!formats.contains(options->dataFormat)) {
        *error = QString("Unsupported --format '%1'. Supported formats: %2")
            .arg(options->dataFormat, formats.join(", "));
        return false;
    }

    if (parser.isSet("pixel-format")) {
        const QString value = parser.value("pixel-format").toUpper();
        const QStringList supported = {"ARGB8888", "ARGB8565", "ARGB4444", "RGB888", "RGB565", "ALPHA"};
        if (!supported.contains(value)) {
            *error = QString("Unsupported --pixel-format '%1'. Supported formats: %2")
                .arg(value, supported.join(", "));
            return false;
        }
        options->pixelFormat = pixelFormatFromString(value);
    }
    return true;
}

bool ensureDirectory(const QString& path, QDir* result, QString* error)
{
    if (path.isEmpty()) {
        *error = "No destination directory was provided.";
        return false;
    }
    QDir directory(QDir::cleanPath(path));
    if (!directory.exists() && !QDir().mkpath(directory.absolutePath())) {
        *error = QString("Cannot create destination directory '%1'.").arg(directory.absolutePath());
        return false;
    }
    *result = directory;
    return true;
}

bool addAtlas(PublishSpriteSheet* publisher,
              const QStringList& sources,
              const QString& outputPath,
              const PackOptions& options,
              QString* error)
{
    if (sources.isEmpty()) {
        *error = "No source images were specified.";
        return false;
    }
    for (const QString& source : sources) {
        if (!QFileInfo::exists(source)) {
            *error = QString("Source '%1' does not exist.").arg(source);
            return false;
        }
    }
    const QFileInfo outputInfo(outputPath);
    if (!QDir().mkpath(outputInfo.absolutePath())) {
        *error = QString("Cannot create output directory '%1'.").arg(outputInfo.absolutePath());
        return false;
    }

    SpriteAtlas atlas(sources,
                      options.textureBorder,
                      options.spriteBorder,
                      options.trim,
                      options.heuristicMask,
                      options.powerOfTwo,
                      options.forceSquared,
                      options.maxSize,
                      options.scale);
    atlas.setRotateSprites(options.rotateSprites);
    atlas.setAlgorithm(options.algorithm);
    if (options.trimMode == "Polygon") {
        atlas.enablePolygonMode(true, options.epsilon);
    }
    if (!atlas.generate()) {
        *error = "Atlas generation failed. The sprites may exceed --max-size.";
        return false;
    }
    publisher->addSpriteSheet(atlas, outputPath);
    return true;
}

} // namespace

int commandLine(QCoreApplication& app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Pack images into PNG, WebP, or JPEG sprite sheets and export matching metadata without a GUI.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("source", "Image/directory to pack, or an .ssp/.json/.tps project file.");
    parser.addPositionalArgument("destination", "Output directory. Optional when a project contains one.", "[destination]");

    parser.addOptions({
        {{"f", "format"}, "Metadata format (see --list-formats).", "format", "cocos2d"},
        {{"o", "output-name"}, "Output base name for direct image/directory input.", "name"},
        {{"trim-mode", "trimMode"}, "Trim mode: Rect or Polygon.", "mode", "Rect"},
        {"algorithm", "Packing algorithm: Rect or Polygon.", "mode", "Rect"},
        {"trim", "Alpha threshold from 0 (disabled) to 255.", "value", "1"},
        {"epsilon", "Polygon simplification epsilon greater than 0.", "value", "5"},
        {"texture-border", "Transparent border around each sheet.", "pixels", "0"},
        {"sprite-border", "Spacing between sprites.", "pixels", "2"},
        {{"power-of-two", "powerOf2"}, "Force power-of-two sheet dimensions."},
        {"force-squared", "Force square sheet dimensions."},
        {"heuristic-mask", "Derive transparency from the image corner color."},
        {"rotate", "Allow 90-degree sprite rotation."},
        {"max-size", "Maximum width and height of a sheet.", "pixels", "8192"},
        {"scale", "Scale source images before packing.", "factor", "1"},
        {"pixel-format", "Texture pixel format: ARGB8888, ARGB8565, ARGB4444, RGB888, RGB565, or ALPHA.", "format", "ARGB8888"},
        {"texture-format", "Texture format: png, webp, or jpg.", "format", "png"},
        {"no-premultiplied", "Disable the project premultiplied-alpha setting."},
        {{"trim-sprite-names", "trimSpriteNames"}, "Remove image extensions from sprite names."},
        {{"prepend-smart-folder-name", "prependSmartFolderName"}, "Keep the top-level source folder in sprite names."},
        {"png-compression", "PNG compression level from 0 to 9.", "level", "9"},
        {"webp-quality", "WebP quality from 0 to 100.", "quality", "80"},
        {"jpg-quality", "JPEG quality from 0 to 100.", "quality", "80"},
        {"pngquant", "Optionally reduce PNG size with the external pngquant executable."},
        {"no-pngquant", "Disable pngquant requested by a project file."},
        {"pngquant-quality", "pngquant quality range in min-max form; also enables pngquant.", "range", "80-95"},
        {"verbose", "Show packing diagnostics."},
        {"list-formats", "List supported metadata formats and exit."},
        {"list-image-formats", "List currently available input image formats and exit."},
        {"list-texture-formats", "List currently available texture output formats and exit."}
    });

    parser.process(app);
    if (!parser.isSet("verbose")) {
        QLoggingCategory::setFilterRules("*.debug=false");
    }
    if (parser.isSet("list-formats")) {
        QStringList formats = DataExporter::supportedFormats();
        formats.append("none");
        qInfo().noquote() << formats.join('\n');
        return 0;
    }
    if (parser.isSet("list-image-formats")) {
        QStringList formats;
        for (const QByteArray& format : QImageReader::supportedImageFormats()) {
            formats.append(QString::fromLatin1(format));
        }
        formats.removeDuplicates();
        formats.sort(Qt::CaseInsensitive);
        qInfo().noquote() << formats.join('\n');
        return 0;
    }
    if (parser.isSet("list-texture-formats")) {
        const QList<QByteArray> writers = QImageWriter::supportedImageFormats();
        QStringList formats;
        if (writers.contains("png")) formats.append("png");
        if (writers.contains("webp")) formats.append("webp");
        if (writers.contains("jpeg") || writers.contains("jpg")) formats.append("jpg");
        qInfo().noquote() << formats.join('\n');
        return 0;
    }
    const QStringList positional = parser.positionalArguments();
    if (positional.isEmpty() || positional.size() > 2) {
        parser.showHelp(2);
    }

    const QFileInfo source(QDir::cleanPath(positional.at(0)));
    if (!source.exists()) {
        printError(QString("Source '%1' does not exist.").arg(source.filePath()));
        return 1;
    }

    QScopedPointer<SpritePackerProjectFile> project = source.isFile() ? openProject(source) : QScopedPointer<SpritePackerProjectFile>();
    if (project && !project->read(source.absoluteFilePath())) {
        printError(QString("Cannot read project file '%1'.").arg(source.filePath()));
        return 1;
    }
    PackOptions options = project ? optionsFromProject(*project) : PackOptions();
    QString error;
    if (!applyOverrides(parser, &options, &error)) {
        printError(error);
        return 2;
    }

    QString destinationPath;
    if (positional.size() == 2) destinationPath = positional.at(1);
    else if (project) destinationPath = project->destPath();

    QDir destination;
    if (!ensureDirectory(destinationPath, &destination, &error)) {
        printError(error);
        return 1;
    }

    PublishSpriteSheet publisher;
    publisher.setImageFormat(options.imageFormat);
    publisher.setPixelFormat(options.pixelFormat);
    publisher.setPremultiplied(options.premultiplied);
    publisher.setTrimSpriteNames(options.trimSpriteNames);
    publisher.setPrependSmartFolderName(options.prependSmartFolderName);
    publisher.setPngCompression(options.pngCompression);
    publisher.setWebpQuality(options.webpQuality);
    publisher.setJpgQuality(options.jpgQuality);
    publisher.setPngQuant(options.pngQuant, options.pngQuantQuality);

    if (project) {
        QVector<ScalingVariant> variants = project->scalingVariants();
        if (variants.isEmpty()) {
            variants.append({QString(), 1.0f, options.maxSize, options.powerOfTwo, options.forceSquared});
        }
        for (const ScalingVariant& variant : variants) {
            PackOptions variantOptions = options;
            if (!parser.isSet("scale")) variantOptions.scale = variant.scale;
            if (!parser.isSet("max-size")) variantOptions.maxSize = qMax(1, variant.maxTextureSize);
            if (!parser.isSet("power-of-two") && !parser.isSet("powerOf2")) variantOptions.powerOfTwo = variant.pow2;
            if (!parser.isSet("force-squared")) variantOptions.forceSquared = variant.forceSquared;

            QString outputName = project->spriteSheetName();
            if (outputName.isEmpty()) outputName = source.completeBaseName();
            if (outputName.contains("{v}")) outputName.replace("{v}", variant.name);
            else if (!variant.name.isEmpty()) outputName.prepend(variant.name);
            while (outputName.startsWith('/')) outputName.remove(0, 1);

            if (!addAtlas(&publisher,
                          project->srcList(),
                          destination.absoluteFilePath(outputName),
                          variantOptions,
                          &error)) {
                printError(error);
                return 1;
            }
        }
    } else {
        QString outputName = parser.value("output-name");
        if (outputName.isEmpty()) {
            outputName = source.isDir() ? source.fileName() : source.completeBaseName();
        }
        if (!addAtlas(&publisher,
                      {source.absoluteFilePath()},
                      destination.absoluteFilePath(outputName),
                      options,
                      &error)) {
            printError(error);
            return 1;
        }
    }

    if (!publisher.publish(options.dataFormat, &error)) {
        printError(error);
        return 1;
    }

    qInfo().noquote() << "Published" << textureFormatName(options.imageFormat).toUpper()
                      << "sprite sheet(s) to" << destination.absolutePath();
    return 0;
}
