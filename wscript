

def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  if not conf.check(lib="getdata",
		    #libpath=['/usr/local/lib', '/usr/lib'],
		    uselib_store="GETDATA"):
    conf.fatal("Can't locate GetData libraries")

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64",
		    "-D_LARGEFILE_SOURCE", "-Wall"]
  #obj.lib = "getdata"
  obj.uselib = "GETDATA"
  obj.target = "getdata_bindings"
  obj.source = "bindings.cc"