# Define all supported languages in one place
# Format: "code|English name|native name"
set(SUPPORTED_LANGUAGES
    "en|English|english"
    "fr|French|français" 
    "de|German|deutsch"
    "it|Italian|italiano"
    "ko|Korean|한국어"
    "ru|Russian|pусский"
    "zh_CN|Chinese (Simplified)|简体中文"
)

# Function to extract language codes only
function(get_language_codes OUTPUT_VAR)
    set(CODES "")
    foreach(LANG_INFO ${SUPPORTED_LANGUAGES})
        string(REPLACE "|" ";" LANG_LIST "${LANG_INFO}")
        list(GET LANG_LIST 0 CODE)
        list(APPEND CODES ${CODE})
    endforeach()
    set(${OUTPUT_VAR} ${CODES} PARENT_SCOPE)
endfunction()

# Function to generate TS file list
function(get_ts_files OUTPUT_VAR PROJECT_NAME)
    get_language_codes(CODES)
    set(TS_FILES "")
    foreach(CODE ${CODES})
        list(APPEND TS_FILES "i18n/${PROJECT_NAME}_${CODE}.ts")
    endforeach()
    set(${OUTPUT_VAR} ${TS_FILES} PARENT_SCOPE)
endfunction()

# Function to generate QM copy commands
function(get_qm_files OUTPUT_VAR PROJECT_NAME)
    get_language_codes(CODES)
    set(QM_FILES "")
    foreach(CODE ${CODES})
        list(APPEND QM_FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_${CODE}.qm")
    endforeach()
    set(${OUTPUT_VAR} ${QM_FILES} PARENT_SCOPE)
endfunction()

# Function to generate languages header
function(generate_languages_header)
    # Generate language data for the header
    set(LANGUAGE_DATA "")
    foreach(LANG_INFO ${SUPPORTED_LANGUAGES})
        string(REPLACE "|" ";" LANG_LIST "${LANG_INFO}")
        list(GET LANG_LIST 0 CODE)
        list(GET LANG_LIST 1 ENGLISH_NAME)
        list(GET LANG_LIST 2 NATIVE_NAME)
        
        string(APPEND LANGUAGE_DATA "        {\"${CODE}\", \"${ENGLISH_NAME}\", \"${NATIVE_NAME}\"},\n")
    endforeach()
    
    # Remove trailing comma and newline
    string(REGEX REPLACE ",\n$" "\n" LANGUAGE_DATA "${LANGUAGE_DATA}")
    
    # Set the variable in parent scope for configure_file
    set(LANGUAGE_DATA "${LANGUAGE_DATA}" PARENT_SCOPE)
    
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/languages.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/languages.h"
        @ONLY
    )
endfunction()
