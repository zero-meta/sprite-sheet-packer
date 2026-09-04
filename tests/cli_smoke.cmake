if(NOT DEFINED CLI OR NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "CLI, INPUT, and OUTPUT are required")
endif()

file(REMOVE_RECURSE "${OUTPUT}")
file(MAKE_DIRECTORY "${OUTPUT}")

execute_process(
    COMMAND "${CLI}" "${INPUT}" "${OUTPUT}" --format json --output-name smoke
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "JSON smoke test failed (${result}):\n${stdout}\n${stderr}")
endif()

foreach(output_file IN ITEMS "${OUTPUT}/smoke.png" "${OUTPUT}/smoke.json")
    if(NOT EXISTS "${output_file}")
        message(FATAL_ERROR "Missing smoke-test output: ${output_file}")
    endif()
endforeach()

file(READ "${OUTPUT}/smoke.png" png_signature LIMIT 8 HEX)
if(NOT png_signature STREQUAL "89504e470d0a1a0a")
    message(FATAL_ERROR "Smoke-test texture is not a PNG")
endif()

execute_process(
    COMMAND "${CLI}" "${INPUT}" "${OUTPUT}"
        --format none
        --output-name uncompressed
        --png-compression 0
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT EXISTS "${OUTPUT}/uncompressed.png")
    message(FATAL_ERROR "Uncompressed PNG smoke test failed (${result}):\n${stdout}\n${stderr}")
endif()

file(SIZE "${OUTPUT}/smoke.png" compressed_png_size)
file(SIZE "${OUTPUT}/uncompressed.png" uncompressed_png_size)
if(NOT compressed_png_size LESS uncompressed_png_size)
    message(FATAL_ERROR
        "PNG compression is ineffective: level 9 produced ${compressed_png_size} bytes, "
        "level 0 produced ${uncompressed_png_size} bytes")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "PATH="
        "${CLI}" "${INPUT}" "${OUTPUT}"
        --format none
        --output-name pngquant-missing
        --pngquant
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT EXISTS "${OUTPUT}/pngquant-missing.png")
    message(FATAL_ERROR
        "Missing pngquant must not fail publishing (${result}):\n${stdout}\n${stderr}")
endif()

execute_process(
    COMMAND "${CLI}" --list-texture-formats
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
set(texture_formats "${stdout}\n${stderr}")
string(REPLACE "\r" "" texture_formats "${texture_formats}")
string(REPLACE "\n" ";" texture_format_list "${texture_formats}")
list(FILTER texture_format_list EXCLUDE REGEX "^$")
if(NOT result EQUAL 0 OR NOT "png" IN_LIST texture_format_list)
    message(FATAL_ERROR "PNG texture writer was not reported (${result}):\n${stderr}")
endif()

if("webp" IN_LIST texture_format_list)
    execute_process(
        COMMAND "${CLI}" "${INPUT}" "${OUTPUT}"
            --format pixijs
            --output-name webp-sheet
            --texture-format webp
            --webp-quality 80
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
    )
    if(NOT result EQUAL 0
       OR NOT EXISTS "${OUTPUT}/webp-sheet.webp"
       OR NOT EXISTS "${OUTPUT}/webp-sheet.json")
        message(FATAL_ERROR "WebP smoke test failed (${result}):\n${stdout}\n${stderr}")
    endif()
    file(READ "${OUTPUT}/webp-sheet.webp" webp_signature LIMIT 12 HEX)
    if(NOT webp_signature MATCHES "^52494646........57454250$")
        message(FATAL_ERROR "WebP smoke-test texture has an invalid signature")
    endif()
    file(READ "${OUTPUT}/webp-sheet.json" webp_json)
    string(JSON webp_image GET "${webp_json}" meta image)
    if(NOT webp_image STREQUAL "webp-sheet.webp")
        message(FATAL_ERROR "WebP metadata contains the wrong texture file name")
    endif()
endif()

if(NOT "jpg" IN_LIST texture_format_list)
    message(FATAL_ERROR "JPEG texture writer was not reported")
endif()
execute_process(
    COMMAND "${CLI}" "${INPUT}" "${OUTPUT}"
        --format pixijs
        --output-name jpg-sheet
        --texture-format jpg
        --jpg-quality 80
        --pixel-format RGB888
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0
   OR NOT EXISTS "${OUTPUT}/jpg-sheet.jpg"
   OR NOT EXISTS "${OUTPUT}/jpg-sheet.json")
    message(FATAL_ERROR "JPEG smoke test failed (${result}):\n${stdout}\n${stderr}")
endif()
file(READ "${OUTPUT}/jpg-sheet.jpg" jpg_signature LIMIT 2 HEX)
if(NOT jpg_signature STREQUAL "ffd8")
    message(FATAL_ERROR "JPEG smoke-test texture has an invalid signature")
endif()
file(READ "${OUTPUT}/jpg-sheet.json" jpg_json)
string(JSON jpg_image GET "${jpg_json}" meta image)
if(NOT jpg_image STREQUAL "jpg-sheet.jpg")
    message(FATAL_ERROR "JPEG metadata contains the wrong texture file name")
endif()

execute_process(
    COMMAND "${CLI}" "${INPUT}" "${OUTPUT}"
        --format none
        --output-name jpg-alpha
        --texture-format jpg
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "sprite pixels contain transparency")
    message(FATAL_ERROR "JPEG transparency validation did not reject an Alpha texture")
endif()

file(READ "${OUTPUT}/smoke.json" json_contents)
string(JSON frame_count LENGTH "${json_contents}")
if(NOT frame_count EQUAL 1)
    message(FATAL_ERROR "Expected one JSON frame, got ${frame_count}")
endif()

execute_process(
    COMMAND "${CLI}" "${INPUT}" "${OUTPUT}"
        --format json
        --output-name extruded
        --sprite-border 0
        --extrude 2
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT EXISTS "${OUTPUT}/extruded.png")
    message(FATAL_ERROR "Edge-extrusion smoke test failed (${result}):\n${stdout}\n${stderr}")
endif()
file(READ "${OUTPUT}/extruded.json" extruded_json)
string(JSON base_frame_width GET "${json_contents}" "icon-addFolder.png" frame width)
string(JSON base_frame_height GET "${json_contents}" "icon-addFolder.png" frame height)
string(JSON extruded_frame_x GET "${extruded_json}" "icon-addFolder.png" frame x)
string(JSON extruded_frame_y GET "${extruded_json}" "icon-addFolder.png" frame y)
string(JSON extruded_frame_width GET "${extruded_json}" "icon-addFolder.png" frame width)
string(JSON extruded_frame_height GET "${extruded_json}" "icon-addFolder.png" frame height)
if(NOT extruded_frame_x EQUAL 2
   OR NOT extruded_frame_y EQUAL 2
   OR NOT extruded_frame_width EQUAL base_frame_width
   OR NOT extruded_frame_height EQUAL base_frame_height)
    message(FATAL_ERROR "Extrusion changed frame metadata instead of surrounding it")
endif()

execute_process(
    COMMAND "${CLI}" "${INPUT}" "${OUTPUT}"
        --format none
        --output-name polygon-extruded
        --trim-mode Polygon
        --algorithm Polygon
        --extrude 1
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "not supported with polygon packing")
    message(FATAL_ERROR "Polygon packing did not reject edge extrusion")
endif()

execute_process(
    COMMAND "${CLI}" "${INPUT}" "${OUTPUT}" --format cocos2d --output-name smoke-plist
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT EXISTS "${OUTPUT}/smoke-plist.plist")
    message(FATAL_ERROR "plist smoke test failed (${result}):\n${stdout}\n${stderr}")
endif()

file(READ "${OUTPUT}/smoke-plist.plist" plist_contents)
if(NOT plist_contents MATCHES "<plist version=\"1.0\">")
    message(FATAL_ERROR "Smoke-test plist is malformed")
endif()

execute_process(
    COMMAND "${CLI}" "${INPUT}" "${OUTPUT}"
        --format json
        --output-name polygon
        --trim-mode Polygon
        --algorithm Polygon
        --heuristic-mask
        --power-of-two
        --force-squared
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0 OR NOT EXISTS "${OUTPUT}/polygon.png")
    message(FATAL_ERROR "Polygon smoke test failed (${result}):\n${stdout}\n${stderr}")
endif()

file(MAKE_DIRECTORY "${OUTPUT}/nested/anim")
file(COPY_FILE "${INPUT}" "${OUTPUT}/nested/anim/walk.png")
foreach(corona_format IN ITEMS corona corona2)
    execute_process(
        COMMAND "${CLI}" "${OUTPUT}/nested" "${OUTPUT}"
            --format "${corona_format}"
            --output-name "${corona_format}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
    )
    if(NOT result EQUAL 0 OR NOT EXISTS "${OUTPUT}/${corona_format}.json")
        message(FATAL_ERROR "${corona_format} smoke test failed (${result}):\n${stdout}\n${stderr}")
    endif()
endforeach()

file(READ "${OUTPUT}/corona.json" corona_json)
string(JSON corona_index GET "${corona_json}" frameIndex "walk.png")
if(NOT corona_index EQUAL 1)
    message(FATAL_ERROR "corona frameIndex did not use the sprite file name")
endif()

file(READ "${OUTPUT}/corona2.json" corona2_json)
string(JSON corona2_index GET "${corona2_json}" frameIndex "anim/walk")
if(NOT corona2_index EQUAL 1)
    message(FATAL_ERROR "corona2 frameIndex did not preserve the direct parent folder")
endif()

file(TO_CMAKE_PATH "${INPUT}" project_input)
file(TO_CMAKE_PATH "${OUTPUT}/nested" project_nested_input)
file(TO_CMAKE_PATH "${OUTPUT}/project-output" project_output)
set(project_texture_format "*.png")
set(project_texture_extension "png")
if("webp" IN_LIST texture_format_list)
    set(project_texture_format "*.webp")
    set(project_texture_extension "webp")
endif()
file(WRITE "${OUTPUT}/smoke.ssp" "{
  \"trimMode\": \"Rect\",
  \"algorithm\": \"Rect\",
  \"imageFormat\": \"${project_texture_format}\",
  \"pixelFormat\": \"ARGB8888\",
  \"extrude\": 0,
  \"extrudeRules\": [
    {\"pattern\": \"**/*.png\", \"pixels\": 1},
    {\"pattern\": \"nested/**\", \"pixels\": 3}
  ],
  \"outlineCoarseness\": 0,
  \"outlineRules\": [
    {\"pattern\": \"icon-addFolder.png\", \"coarseness\": 2},
    {\"pattern\": \"nested/**\", \"coarseness\": 2, \"centered\": true}
  ],
  \"pngOptMode\": \"Lossy\",
  \"pngQuantQuality\": \"80-95\",
  \"dataFormat\": \"pixijs\",
  \"destPath\": \"${project_output}\",
  \"spriteSheetName\": \"{v}project\",
  \"srcList\": [\"${project_input}\", \"${project_nested_input}\"],
  \"scalingVariants\": [{
    \"name\": \"@1x-\",
    \"scale\": 1,
    \"maxTextureSize\": 1024,
    \"pow2\": true,
    \"forceSquared\": true
  }],
  \"textureScaleVariants\": [
    {\"scale\": 0.75, \"suffix\": \"@1080p\"},
    {\"scale\": 0.5, \"suffix\": \"@other\"}
  ]
}")

execute_process(
    COMMAND "${CLI}" "${OUTPUT}/smoke.ssp"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0
   OR NOT EXISTS "${OUTPUT}/project-output/@1x-project.${project_texture_extension}"
   OR NOT EXISTS "${OUTPUT}/project-output/@1x-project@1080p.${project_texture_extension}"
   OR NOT EXISTS "${OUTPUT}/project-output/@1x-project@other.${project_texture_extension}"
   OR NOT EXISTS "${OUTPUT}/project-output/@1x-project.json")
    message(FATAL_ERROR "Project smoke test failed (${result}):\n${stdout}\n${stderr}")
endif()
if(EXISTS "${OUTPUT}/project-output/@1x-project@1080p.json"
   OR EXISTS "${OUTPUT}/project-output/@1x-project@other.json")
    message(FATAL_ERROR "textureScaleVariants unexpectedly generated metadata files")
endif()

foreach(texture_variant IN ITEMS "@1x-project" "@1x-project@1080p" "@1x-project@other")
    execute_process(
        COMMAND "${CLI}"
            "${OUTPUT}/project-output/${texture_variant}.${project_texture_extension}"
            "${OUTPUT}/variant-inspection"
            --format json
            --output-name "${texture_variant}"
            --trim 0
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Cannot inspect texture variant ${texture_variant} (${result}):\n${stdout}\n${stderr}")
    endif()
    file(READ "${OUTPUT}/variant-inspection/${texture_variant}.json" texture_variant_json)
    string(JSON texture_variant_width GET "${texture_variant_json}"
        "${texture_variant}.${project_texture_extension}" sourceSize width)
    string(JSON texture_variant_height GET "${texture_variant_json}"
        "${texture_variant}.${project_texture_extension}" sourceSize height)
    if(texture_variant STREQUAL "@1x-project")
        set(base_texture_width ${texture_variant_width})
        set(base_texture_height ${texture_variant_height})
    elseif(texture_variant STREQUAL "@1x-project@1080p")
        set(texture_1080p_width ${texture_variant_width})
        set(texture_1080p_height ${texture_variant_height})
    else()
        set(texture_other_width ${texture_variant_width})
        set(texture_other_height ${texture_variant_height})
    endif()
endforeach()
math(EXPR expected_1080p_width "(${base_texture_width} * 3 + 2) / 4")
math(EXPR expected_1080p_height "(${base_texture_height} * 3 + 2) / 4")
math(EXPR expected_other_width "(${base_texture_width} + 1) / 2")
math(EXPR expected_other_height "(${base_texture_height} + 1) / 2")
if(NOT texture_1080p_width EQUAL expected_1080p_width
   OR NOT texture_1080p_height EQUAL expected_1080p_height
   OR NOT texture_other_width EQUAL expected_other_width
   OR NOT texture_other_height EQUAL expected_other_height)
    message(FATAL_ERROR "textureScaleVariants did not scale the completed scale=1 atlas")
endif()

execute_process(
    COMMAND "${CLI}" "${OUTPUT}/smoke.ssp" "${OUTPUT}/project-no-unit-scale" --scale 0.5
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "require exactly one scale=1 atlas output")
    message(FATAL_ERROR "textureScaleVariants did not reject a project without a scale=1 atlas")
endif()

file(READ "${OUTPUT}/project-output/@1x-project.json" project_json)
string(JSON project_frame_count LENGTH "${project_json}" frames)
if(NOT project_frame_count EQUAL 2)
    message(FATAL_ERROR "Project srcList did not accept its mixed file/directory inputs")
endif()
string(JSON project_frame_x GET "${project_json}" frames "./icon-addFolder" frame x)
string(JSON project_frame_y GET "${project_json}" frames "./icon-addFolder" frame y)
string(JSON project_alias_frame_x GET "${project_json}" frames "nested/anim/walk" frame x)
string(JSON project_alias_frame_y GET "${project_json}" frames "nested/anim/walk" frame y)
if(NOT project_frame_x EQUAL 3
   OR NOT project_frame_y EQUAL 3
   OR NOT project_alias_frame_x EQUAL project_frame_x
   OR NOT project_alias_frame_y EQUAL project_frame_y)
    message(FATAL_ERROR "Project extrudeRules or identical-frame maximum was not applied")
endif()
string(JSON project_image GET "${project_json}" meta image)
if(NOT project_image STREQUAL "@1x-project.${project_texture_extension}")
    message(FATAL_ERROR "Project imageFormat was not used by the texture writer")
endif()

execute_process(
    COMMAND "${CLI}" "${OUTPUT}/smoke.ssp" "${OUTPUT}/project-override" --extrude 0
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Project extrusion override failed (${result}):\n${stdout}\n${stderr}")
endif()
file(READ "${OUTPUT}/project-override/@1x-project.json" override_json)
string(JSON override_frame_x GET "${override_json}" frames "./icon-addFolder" frame x)
string(JSON override_frame_y GET "${override_json}" frames "./icon-addFolder" frame y)
if(NOT override_frame_x EQUAL 0 OR NOT override_frame_y EQUAL 0)
    message(FATAL_ERROR "Explicit --extrude did not override project extrudeRules")
endif()

execute_process(
    COMMAND "${CLI}" "${OUTPUT}/smoke.ssp" "${OUTPUT}/project-corona2"
        --format corona2
        --extrude 0
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Mixed-source corona2 export failed (${result}):\n${stdout}\n${stderr}")
endif()
file(READ "${OUTPUT}/project-corona2/@1x-project.json" mixed_corona2_json)
string(JSON mixed_file_index GET "${mixed_corona2_json}" frameIndex "icon-addFolder")
string(JSON mixed_directory_index GET "${mixed_corona2_json}" frameIndex "nested/anim/walk")
if(mixed_file_index LESS 1 OR mixed_directory_index LESS 1)
    message(FATAL_ERROR "corona2 did not preserve mixed file/directory logical paths")
endif()

math(EXPR mixed_file_frame "${mixed_file_index} - 1")
math(EXPR mixed_directory_frame "${mixed_directory_index} - 1")
string(JSON mixed_file_outline ERROR_VARIABLE mixed_file_outline_error
    GET "${mixed_corona2_json}" sheet frames ${mixed_file_frame} outline)
string(JSON mixed_directory_outline ERROR_VARIABLE mixed_directory_outline_error
    GET "${mixed_corona2_json}" sheet frames ${mixed_directory_frame} outline)
if(NOT mixed_file_outline_error STREQUAL "NOTFOUND")
    message(FATAL_ERROR "outlineRules did not generate the uncentered comparison outline")
endif()
if(NOT mixed_directory_outline_error STREQUAL "NOTFOUND")
    message(FATAL_ERROR "outlineRules did not generate an outline for the matched directory frame")
endif()
string(JSON mixed_file_outline_length LENGTH "${mixed_file_outline}")
string(JSON mixed_directory_outline_length LENGTH "${mixed_directory_outline}")
math(EXPR mixed_directory_outline_remainder "${mixed_directory_outline_length} % 2")
if(NOT mixed_file_outline_length EQUAL mixed_directory_outline_length
   OR mixed_directory_outline_length LESS 6
   OR NOT mixed_directory_outline_remainder EQUAL 0)
    message(FATAL_ERROR "Generated outline does not contain at least three coordinate pairs")
endif()
string(JSON centered_frame_width GET "${mixed_corona2_json}"
    sheet frames ${mixed_directory_frame} width)
string(JSON centered_frame_height GET "${mixed_corona2_json}"
    sheet frames ${mixed_directory_frame} height)
math(EXPR centered_offset_x "${centered_frame_width} / 2")
math(EXPR centered_offset_y "${centered_frame_height} / 2")
math(EXPR mixed_outline_last "${mixed_directory_outline_length} - 1")
foreach(coordinate_index RANGE 0 ${mixed_outline_last})
    string(JSON uncentered_coordinate GET "${mixed_file_outline}" ${coordinate_index})
    string(JSON centered_coordinate GET "${mixed_directory_outline}" ${coordinate_index})
    math(EXPR coordinate_axis "${coordinate_index} % 2")
    if(coordinate_axis EQUAL 0)
        math(EXPR expected_centered_coordinate "${uncentered_coordinate} - ${centered_offset_x}")
    else()
        math(EXPR expected_centered_coordinate "${uncentered_coordinate} - ${centered_offset_y}")
    endif()
    if(NOT centered_coordinate EQUAL expected_centered_coordinate)
        message(FATAL_ERROR "centered outline was not offset from the trimmed frame center")
    endif()
endforeach()

get_filename_component(resource_directory "${INPUT}" DIRECTORY)
file(TO_CMAKE_PATH "${resource_directory}/icon-lock.png" odd_outline_input)
file(TO_CMAKE_PATH "${OUTPUT}/centered-outline-output" centered_outline_output)
file(WRITE "${OUTPUT}/centered-outline.ssp" "{
  \"trimMode\": \"Rect\",
  \"algorithm\": \"Rect\",
  \"imageFormat\": \"*.png\",
  \"dataFormat\": \"corona2\",
  \"destPath\": \"${centered_outline_output}\",
  \"spriteSheetName\": \"centered-outline\",
  \"srcList\": [\"${odd_outline_input}\"],
  \"outlineRules\": [
    {\"pattern\": \"icon-lock.png\", \"coarseness\": 2, \"centered\": true}
  ]
}")
execute_process(
    COMMAND "${CLI}" "${OUTPUT}/centered-outline.ssp"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Odd-sized centered outline export failed (${result}):\n${stdout}\n${stderr}")
endif()
file(READ "${OUTPUT}/centered-outline-output/centered-outline.json" odd_outline_json)
string(JSON odd_outline_index GET "${odd_outline_json}" frameIndex "icon-lock")
math(EXPR odd_outline_frame "${odd_outline_index} - 1")
string(JSON odd_outline GET "${odd_outline_json}" sheet frames ${odd_outline_frame} outline)
string(JSON odd_outline_length LENGTH "${odd_outline}")
math(EXPR odd_outline_last "${odd_outline_length} - 1")
set(found_half_pixel false)
foreach(coordinate_index RANGE 0 ${odd_outline_last})
    string(JSON centered_coordinate GET "${odd_outline}" ${coordinate_index})
    if(centered_coordinate MATCHES "\\.5$")
        set(found_half_pixel true)
        break()
    endif()
endforeach()
if(NOT found_half_pixel)
    message(FATAL_ERROR "Centered outline for an odd-sized frame did not preserve half-pixel coordinates")
endif()

execute_process(
    COMMAND "${CLI}" "${odd_outline_input}" "${OUTPUT}/cli-centered-outline"
        --format corona2
        --output-name cli-centered-outline
        --outline-coarseness 2
        --outline-origin center
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "CLI centered outline export failed (${result}):\n${stdout}\n${stderr}")
endif()
file(READ "${OUTPUT}/cli-centered-outline/cli-centered-outline.json" cli_centered_json)
string(JSON cli_centered_index GET "${cli_centered_json}" frameIndex "icon-lock")
math(EXPR cli_centered_frame "${cli_centered_index} - 1")
string(JSON cli_centered_outline GET "${cli_centered_json}"
    sheet frames ${cli_centered_frame} outline)
if(NOT cli_centered_outline STREQUAL odd_outline)
    message(FATAL_ERROR "--outline-origin center did not match centered project output")
endif()

execute_process(
    COMMAND "${CLI}" "${OUTPUT}/centered-outline.ssp" "${OUTPUT}/top-left-outline"
        --outline-origin top-left
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "CLI top-left outline override failed (${result}):\n${stdout}\n${stderr}")
endif()
file(READ "${OUTPUT}/top-left-outline/centered-outline.json" top_left_json)
string(JSON top_left_index GET "${top_left_json}" frameIndex "icon-lock")
math(EXPR top_left_frame "${top_left_index} - 1")
string(JSON top_left_outline GET "${top_left_json}" sheet frames ${top_left_frame} outline)
string(JSON top_left_outline_length LENGTH "${top_left_outline}")
math(EXPR top_left_outline_last "${top_left_outline_length} - 1")
foreach(coordinate_index RANGE 0 ${top_left_outline_last})
    string(JSON top_left_coordinate GET "${top_left_outline}" ${coordinate_index})
    if(top_left_coordinate LESS 0 OR top_left_coordinate MATCHES "\\.5$")
        message(FATAL_ERROR "--outline-origin top-left did not override centered project rules")
    endif()
endforeach()

execute_process(
    COMMAND "${CLI}" "${odd_outline_input}" "${OUTPUT}/invalid-outline-origin"
        --outline-coarseness 2
        --outline-origin middle
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(result EQUAL 0 OR NOT stderr MATCHES "must be center or top-left")
    message(FATAL_ERROR "Invalid --outline-origin value was not rejected")
endif()

execute_process(
    COMMAND "${CLI}" "${OUTPUT}/smoke.ssp" "${OUTPUT}/project-outline-override"
        --format corona2
        --extrude 0
        --outline-coarseness 0
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Project outline override failed (${result}):\n${stdout}\n${stderr}")
endif()
file(READ "${OUTPUT}/project-outline-override/@1x-project.json" outline_override_json)
string(JSON outline_override_index GET "${outline_override_json}" frameIndex "nested/anim/walk")
math(EXPR outline_override_frame "${outline_override_index} - 1")
string(JSON outline_override_value ERROR_VARIABLE outline_override_error
    GET "${outline_override_json}" sheet frames ${outline_override_frame} outline)
if(outline_override_error STREQUAL "NOTFOUND")
    message(FATAL_ERROR "Explicit --outline-coarseness 0 did not override project outlineRules")
endif()
