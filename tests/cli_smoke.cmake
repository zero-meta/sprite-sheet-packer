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
  \"extrude\": 1,
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
  }]
}")

execute_process(
    COMMAND "${CLI}" "${OUTPUT}/smoke.ssp"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0
   OR NOT EXISTS "${OUTPUT}/project-output/@1x-project.${project_texture_extension}"
   OR NOT EXISTS "${OUTPUT}/project-output/@1x-project.json")
    message(FATAL_ERROR "Project smoke test failed (${result}):\n${stdout}\n${stderr}")
endif()

file(READ "${OUTPUT}/project-output/@1x-project.json" project_json)
string(JSON project_frame_count LENGTH "${project_json}" frames)
if(NOT project_frame_count EQUAL 2)
    message(FATAL_ERROR "Project srcList did not accept its mixed file/directory inputs")
endif()
string(JSON project_frame_x GET "${project_json}" frames "./icon-addFolder" frame x)
string(JSON project_frame_y GET "${project_json}" frames "./icon-addFolder" frame y)
if(project_frame_x LESS 1 OR project_frame_y LESS 1)
    message(FATAL_ERROR "Project extrude setting was not applied")
endif()
string(JSON project_image GET "${project_json}" meta image)
if(NOT project_image STREQUAL "@1x-project.${project_texture_extension}")
    message(FATAL_ERROR "Project imageFormat was not used by the texture writer")
endif()
