#include <QCoreApplication>

int commandLine(QCoreApplication& app);

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setOrganizationName("amakaseev");
    QCoreApplication::setOrganizationDomain("spicyminds-lab.com");
    QCoreApplication::setApplicationName("sprite-sheet-packer");
    QCoreApplication::setApplicationVersion("1.1.0");

    return commandLine(app);
}
