from conan import ConanFile
from conan.tools.cmake import cmake_layout

class RapidDeskConan(ConanFile):
    name = "rapiddesk"
    version = "1.0.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("qt/6.6.2")
        self.requires("openssl/3.2.1")
        #self.requires("ffmpeg/7.1.3")
        self.requires("boost/1.84.0")
        self.requires("gtest/1.14.0")

    def configure(self):
        # Qt6 - desabilitar PostgreSQL para evitar erro de toolset v143
        self.options["qt"].shared = True
        self.options["qt"].widgets = True
        self.options["qt"].gui = True
        self.options["qt"].network = True
        self.options["qt"].opengl = "no"
        self.options["qt"].with_pq = False      # ← DESABILITAR PostgreSQL
        self.options["qt"].with_mysql = False   # ← DESABILITAR MySQL
        self.options["qt"].with_odbc = False    # ← DESABILITAR ODBC
        self.options["qt"].with_sqlite = True   # manter SQLite (leve)

    def layout(self):
        cmake_layout(self)

    def validate(self):
        from conan.errors import ConanInvalidConfiguration
        if self.settings.os != "Windows":
            raise ConanInvalidConfiguration("Windows-only")