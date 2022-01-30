# pooptracer, a low effort raytracer

### Dependency matrix:

|  Name    |  Version   |  License              |
|---	   |---	        |---	                |
|   SDL2   |  2.0.10    |  zlib                 |
|   glm    |  0.9.9.7   |  Happy bunny / MIT    |
|   fmt    |  6.1.2     |  MIT                  |

### TODO-s

* Scene defined in YML / Json
* Move raytracing core to separate target, add benchmark subtarget
* Extend SceneEntity API - `rotate` method
* Extend SceneEntity API - optional lambda defining transformation
* Extend SceneEntity API - `update` method which would apply optional lambda above
* Fixed timestep - lock FPS
* Math is a little shady, make small reproducible scenes (defined in YML) to verify each part 
* Better color blending & Reflectivity modifier
* Framebuffer postprocessing
* Light source - add skybox in form of a plane above, if skybox hit, modify colors accordingly

### License:

Do whatever you want.
