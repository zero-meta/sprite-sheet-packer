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

file(READ "${OUTPUT}/smoke.json" json_contents)
string(JSON frame_count LENGTH "${json_contents}")
if(NOT frame_count EQUAL 1)
    message(FATAL_ERROR "Expected one JSON frame, got ${frame_count}")
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
file(TO_CMAKE_PATH "${OUTPUT}/project-output" project_output)
file(WRITE "${OUTPUT}/smoke.ssp" "{
  \"trimMode\": \"Rect\",
  \"algorithm\": \"Rect\",
  \"imageFormat\": \"*.png\",
  \"pixelFormat\": \"ARGB8888\",
  \"dataFormat\": \"pixijs\",
  \"destPath\": \"${project_output}\",
  \"spriteSheetName\": \"{v}project\",
  \"srcList\": [\"${project_input}\"],
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
   OR NOT EXISTS "${OUTPUT}/project-output/@1x-project.png"
   OR NOT EXISTS "${OUTPUT}/project-output/@1x-project.json")
    message(FATAL_ERROR "Project smoke test failed (${result}):\n${stdout}\n${stderr}")
endif()
