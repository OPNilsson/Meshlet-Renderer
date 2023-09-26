-- enums
PRESENTATIONMODE = {
	DESKTOP = 1,
	OPENVR = 2,
	OPENXR = 3,
	NONE = 4
}

ENGINEMODE = {
	RASTERIZATION = 1,
	COMPUTE = 2,
	RAYTRACING = 3
}

-- Available Models
-- model = "../models/armadillo.obj"
model = "../models/cube.obj"
--  model = "../models/teapot.obj"
--  model = "../models/bunny.obj"
-- model = "../models/flat_vase.obj"
-- model = "../models/Nefertiti.obj"


-- general options
runtime = 0
filename = 'no_name'

-- LOD options 
-- These options are assumed to change throughout the execution of the code as such setting them here are just an initial options 
SIMPLIFIED = false;  -- If the simplified models should be created otherwise if false the models are loaded from binaries
MAX_LOD = 10; -- The maximum number of LODs that will be created