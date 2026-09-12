# GLEW resolves even core GL 1.0 functions like glGetString via
# glXGetProcAddressARB(), which some GLX implementations (Termux:X11's
# Mesa/llvmpipe layer among them) are allowed to return NULL for, since
# these are meant to be linked directly rather than looked up as
# extensions. That makes glewInit() fail immediately with "Missing GL
# version" even though glGetString(GL_VERSION) works fine when called
# directly. Patch this one lookup to use the statically linked symbol.
set(_file "src/glew.c")
file(READ "${_file}" _contents)

string(FIND "${_contents}" "glewGetProcAddress((const GLubyte*)\"glGetString\")" _pos)
if(_pos EQUAL -1)
	message(FATAL_ERROR "patch-glew-getstring.cmake: expected text not found in ${_file}")
endif()

string(REPLACE
       "glewGetProcAddress((const GLubyte*)\"glGetString\")"
	"glGetString"
	_contents "${_contents}")

file(WRITE "${_file}" "${_contents}")
