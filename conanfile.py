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
        self.requires("ffmpeg/6.1")
        self.requires("boost/1.84.0")
        self.requires("gtest/1.14.0")

    def configure(self):
        # Qt6 config
        self.options["qt"].shared = True
        self.options["qt"].widgets = True
        self.options["qt"].gui = True
        self.options["qt"].network = True
        self.options["qt"].opengl = "no"
        
        # FFmpeg config - CORRIGIDO v3:
        # avformat=True (SSL), avfilter=True (freetype), swresample=True
        self.options["ffmpeg"].shared = True
        self.options["ffmpeg"].avcodec = True
        self.options["ffmpeg"].avutil = True
        self.options["ffmpeg"].swscale = True
        self.options["ffmpeg"].avformat = True
        self.options["ffmpeg"].avfilter = True      # ← ATIVADO (freetype precisa)
        self.options["ffmpeg"].postproc = False
        self.options["ffmpeg"].swresample = True    # ← ATIVADO (pode ser necessario)
        self.options["ffmpeg"].with_libx264 = True
        self.options["ffmpeg"].with_libx265 = False
        self.options["ffmpeg"].with_libvpx = False
        self.options["ffmpeg"].with_libmp3lame = False
        self.options["ffmpeg"].with_libfdk_aac = False
        self.options["ffmpeg"].with_libwebp = False
        self.options["ffmpeg"].with_openjpeg = False
        self.options["ffmpeg"].with_vorbis = False
        self.options["ffmpeg"].with_opus = False
        self.options["ffmpeg"].with_zlib = False
        self.options["ffmpeg"].with_bzip2 = False
        self.options["ffmpeg"].with_libpng = False
        self.options["ffmpeg"].with_libtiff = False
        
        # Boost config - CORRIGIDO v3:
        # exception=True (thread precisa), container=True (cobalt precisa)
        self.options["boost"].shared = False
        self.options["boost"].without_atomic = False
        self.options["boost"].without_chrono = False
        self.options["boost"].without_container = False
        self.options["boost"].without_context = True
        self.options["boost"].without_contract = True
        self.options["boost"].without_coroutine = True
        self.options["boost"].without_date_time = False
        self.options["boost"].without_exception = False  # ← ATIVADO (thread precisa)
        self.options["boost"].without_fiber = True
        self.options["boost"].without_filesystem = False
        self.options["boost"].without_graph = True
        self.options["boost"].without_graph_parallel = True
        self.options["boost"].without_iostreams = True
        self.options["boost"].without_json = True
        self.options["boost"].without_locale = True
        self.options["boost"].without_log = True
        self.options["boost"].without_math = True
        self.options["boost"].without_mpi = True
        self.options["boost"].without_nowide = True
        self.options["boost"].without_program_options = True
        self.options["boost"].without_python = True
        self.options["boost"].without_random = True
        self.options["boost"].without_regex = True
        self.options["boost"].without_serialization = True
        self.options["boost"].without_stacktrace = True
        self.options["boost"].without_system = False
        self.options["boost"].without_test = True
        self.options["boost"].without_thread = False
        self.options["boost"].without_timer = True
        self.options["boost"].without_type_erasure = True
        self.options["boost"].without_wave = True

    def layout(self):
        cmake_layout(self)

    def validate(self):
        if self.settings.os != "Windows":
            raise ConanInvalidConfiguration("This version is Windows-only")