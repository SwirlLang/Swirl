set(CPACK_GENERATOR "NSIS;RPM;DEB")

set(CPACK_PACKAGE_NAME "Swirl")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Swirl")
set(CPACK_PACKAGE_VENDOR "Swirl Lang")
set(CPACK_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Swirl compiler")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/resources/LICENSE")

# RPM config
set(CPACK_RPM_PACKAGE_NAME "Swirl")
set(CPACK_RPM_PACKAGE_DESCRIPTION "Swirl is a modern, general-purpose programming language designed to meet the evolving needs of today's developers. It is a high-level, statically typed, compiled, and object-oriented language that emphasizes both readability and performance.")
set(CPACK_RPM_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION}")
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
set(CPACK_RPM_PACKAGE_URL "https://swirl-lang.vercel.app")

# DEB config
set(CPACK_DEBIAN_PACKAGE_NAME "Swirl")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Swirl Lang")

# NSIS config
set(CPACK_NSIS_PACKAGE_NAME "Swirl")
set(CPACK_NSIS_DISPLAY_NAME "Swirl")
set(CPACK_NSIS_CONTACT "Swirl Lang")
set(CPACK_NSIS_BRANDING_TEXT "Swirl Lang")
set(CPACK_NSIS_URL_INFO_ABOUT "https://swirl-lang.vercel.app")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_MENU_LINKS "https://swirl-lang.vercel.app" "Swirl website" "https://github.com/SwirlLang/Swirl" "Swirl GitHub repo")

include(CPack)
