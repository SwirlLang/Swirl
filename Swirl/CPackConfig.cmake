set(CPACK_GENERATOR "RPM;DEB")

set(CPACK_PACKAGE_NAME "Swirl")
set(CPACK_PACKAGE_VENDOR "Swirl Lang")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Swirl compiler")
set(CPACK_RPM_PACKAGE_DESCRIPTION "Swirl is a modern, general-purpose programming language designed to meet the evolving needs of today's developers. It is a high-level, statically typed, compiled, and object-oriented language that emphasizes both readability and performance.")
set(CPACK_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION}")

# RPM config
set(CPACK_RPM_PACKAGE_NAME "Swirl")
set(CPACK_RPM_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION}")
set(CPACK_RPM_PACKAGE_RELEASE "1")
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
set(CPACK_RPM_PACKAGE_URL "https://swirl-lang.vercel.app")

# DEB config
set(CPACK_DEBIAN_PACKAGE_NAME "Swirl")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Swirl Lang")


include(CPack)
