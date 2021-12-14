//
//  Camera.h
//  example-metalcam
//
//  Created by Joseph Chow on 7/26/18.
//

#ifndef Camera_h
#define Camera_h

#include "MetalCam.h"
#include "ofMain.h"
#include "ofxiOS.h"
#include "ARUtils.h"

#define STRINGIFY(A) #A

namespace ofxARKit {
    namespace core {
        
        typedef std::shared_ptr<class Camera> CameraRef;

        class Camera : public MetalCamRenderer{
            ofShader shader;
            ofMesh mesh;
            
            //MetalCamView * _view;
            //ARSession * session;
            CGRect viewport;
            
            UIInterfaceOrientation orientation;
            ofxARKit::common::ARCameraMatrices cameraMatrices;
            float near,far;
            
        public:
            //! The current tracking state of the camera
            ARTrackingState trackingState;
            //! The reason for when a tracking state might be limited.
            ARTrackingStateReason trackingStateReason;
            //! Flag for turning debug mode on/off
            bool debugMode;
            Camera(ARSession * session);
            static CameraRef create(ARSession * session){
                return CameraRef(new Camera(session));
            }
            
            CVOpenGLESTextureRef getTexture();
                        
            
        /******** MATTE STUFF ***********/
        #if defined( __IPHONE_13_0 )

            // returns a default matte shader. 
            ofShader getDefaultMatteShader(){
                if(!this->session.configuration.frameSemantics == ARFrameSemanticPersonSegmentationWithDepth){
                    return ofShader();
                }
                
                ofShader shd;
                shd.setupShaderFromSource(GL_VERTEX_SHADER, vertexMatte);
                shd.setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentMatte);

                shd.linkProgram();
                return shd;
            }

            CVOpenGLESTextureRef getTextureMatteAlpha();
            CVOpenGLESTextureRef getTextureMatteDepth();
            CVOpenGLESTextureRef getTextureDepth();
            ofMatrix3x3 getAffineTransform();

             //! returns vertex shader source suitable for doing things with person occlusion
            std::string getDefaultMatteVertexShader(){ return vertexMatte; }

                //! a helper to allow you to more easily integrate your own shader source with person occlusion.
                //! @param source the main portion of the shader you want to create
                //! @return chunk the final composed shader source.
            std::string composeMatteCompatibleFragmentShader(std::string source){
                std::string chunk = STRINGIFY(
                                                  
                    precision highp float;
                    varying vec2 vUv;
                    varying vec2 vUvCam;
                                             
                    uniform sampler2D tex;
                    uniform sampler2D texAlphaBody;
                    uniform sampler2D texDepthBody;
                    uniform sampler2D texDepth;
                                             
                    uniform mat4 u_CameraProjectionMat;
                                             
                    uniform float u_time;
                
                    // get the depth, texture and texture depth info from camera feed
                    void main(){
                    
                        vec4 sceneColor = texture2D(tex, vUv);
                        float sceneDepth = texture2D(texDepth, vUvCam).r;
                        
                       
                        float alpha = texture2D( texAlphaBody, vUvCam).r;
                        float dilatedLinearDepth = texture2D(texDepthBody, vUvCam).r;

                        float dilatedDepth = clamp((u_CameraProjectionMat[2][2] * - dilatedLinearDepth + u_CameraProjectionMat[3][2]) / (u_CameraProjectionMat[2][3] * -dilatedLinearDepth + u_CameraProjectionMat[3][3]), 0.0, 1.0);
                                                        
                        float showOccluder = step(dilatedDepth, sceneDepth); // forwardZ case
                );
                        chunk += source;
                            chunk += "}";
                        return chunk;
                    }

              //! Renders the camera image along with a segmented person image. 
            //! intended to be used for debugging purposes. 
            void drawDebugPersonSegmentation();
            bool debugMatteShaderBuilt;
        #endif
            
            ARTrackingStateReason getTrackingState();
            
            //! returns the projection matrix
            glm::mat4 getProjectionMatrix();

            //! Returns the view matrix 
            glm::mat4 getViewMatrix();

            //! Returns the transform matrix. 
            glm::mat4 getTransformMatrix();
            
            //! Returns the current set of camera matrices for the given device orientation.
            ofxARKit::common::ARCameraMatrices getMatricesForOrientation(UIInterfaceOrientation orientation,float near, float far);
            
            //! Returns the current set of camera matrices
            ofxARKit::common::ARCameraMatrices getCameraMatrices();

            //! Update the interface orientation of the device. 
            void updateInterfaceOrientation(int newOrientation);

            //! Sets the current camera matrices as the current global camera projection and view matrices. 
            void setARCameraMatrices();

            //! Logs the current tracking state of the camera. 
            void logTrackingState();

            //! Updates the camera view
            //! @deprecated ?
            void update();

            //! Render the camera image
            void draw();
                        
            private:
                /*
                std::vector<CVImageBufferRef> testVector;
                std::vector<CVImageBufferRef> textMatteAlphaVector;
                std::vector<CVImageBufferRef> textMatteDepthVector;
                int nextIndexToWrite;
                */
                                              
                std::string vertex = STRINGIFY(

                    attribute vec2 position;
                    varying vec2 vUv;
                                                          
                    const vec2 scale = vec2(0.5,0.5);
                    void main(){
                        vec2 uV = position.xy * scale + scale;
                        vUv = vec2(uV.s, 1.0 - uV.t);
                        gl_Position = vec4(position,0.0,1.0);
                });
                            
                std::string fragment = STRINGIFY(
                    precision highp float;
                    varying vec2 vUv;
                                                             
                    uniform sampler2D tex;
                    void main(){
                        gl_FragColor = texture2D(tex, vUv);
                    }
                );
                            
                #if defined( __IPHONE_13_0 )
                    std::string vertexMatte = STRINGIFY(
                                                  
                       attribute vec2 position;

                       uniform vec4 cAffineCamABCD;
                       uniform vec2 cAffineCamTxTy;
               
                       varying vec2 vUv;
                       varying vec2 vUvCam;

                       // https://developer.apple.com/documentation/coregraphics/cgaffinetransform
                       vec2 affineTransform(vec2 uv, vec4 coeff, vec2 offset){
                            return vec2(uv.s * coeff.x + uv.t * coeff.z + offset.x,
                                        uv.s * coeff.y + uv.t * coeff.w + offset.y);
                       }
              
                       const vec2 scale = vec2(0.5,0.5);
                       void main(){
                           
                           
                            vec2 uV = position.xy * scale + scale;
                            vUv = vec2(uV.s, 1.0 - uV.t);
                            vUvCam = affineTransform(vUv, cAffineCamABCD, cAffineCamTxTy);
                                                                                   
                            gl_Position = vec4(position,0.0,1.0);
                           
                           
                           
                    });
                            
                    std::string fragmentMatte = STRINGIFY(
                         precision highp float;
                         varying vec2 vUv;
                         varying vec2 vUvCam;
                 
                         uniform sampler2D tex;
                         uniform sampler2D texAlphaBody;
                         uniform sampler2D texDepthBody;
                         uniform sampler2D texDepth;
                                                          
                                                          
                         
                        
                        uniform mat4 u_CameraProjectionMat;
                 
                        uniform float u_time;
                        const float fluid_speed = 4.25;
                        float rand(float co) { return fract(sin(co*(91.3458)) * 47453.5453); }

                         void main(){
                             
                             vec4 sceneColor = texture2D(tex, vUv);
//                             vec4 sceneColor2 = texture(tex, vUv);
                             
                             float sceneDepth = texture2D(texDepth, vUvCam).r;
                             
                             
                             
                             
                             
                             
                             vec2 newUv = vUv;
                             float alpha = texture2D( texAlphaBody, vUvCam).r;
                             float dilatedLinearDepth = texture2D(texDepthBody, vUvCam).r;
                             float dilatedDepth = clamp((u_CameraProjectionMat[2][2] * - dilatedLinearDepth + u_CameraProjectionMat[3][2]) / (u_CameraProjectionMat[2][3] * -dilatedLinearDepth + u_CameraProjectionMat[3][3]), 0.0, 1.0);
                             float showOccluder = step(dilatedDepth, sceneDepth); // forwardZ case
                             
                             
                             //MAKE THIS FOR PIXEL
                             vec2 texel = 1. / newUv.xy;
                             float step_y = texel.y*(rand(vUv.x)/6.0)*sin(sin(u_time)*.05)*4.0 +.0;
                             float testDot = dot(sceneColor, vec4(.299, .587, .114, 0.0));
                             float compareDot = 1.2*(sin(u_time)*.325+.5);
                             
                             if (testDot > compareDot) {
                                 newUv.y = newUv.y + step_y;
                             } else {
                                 newUv.y = newUv.y - step_y;
                             }
                             vec4 sceneColorSort = texture2D(tex, newUv);
                             vec4 occluderSort = mix(sceneColor, sceneColorSort, alpha);
//                             sceneColor = texture2D(tex, newUv);
                   
                            
                             
                             
                             /**PIXELATE **/
                             
                         
                             float tiles = 80.0;
                             vec2 sceneTiles2 = vUv*tiles;
                             sceneTiles2 = floor(sceneTiles2);
                             sceneTiles2 = sceneTiles2/tiles;
                             
                             vec4 newTex = texture2D(tex, sceneTiles2);
                             // take newTex and matte it
                             vec4 newCamColorTex = newTex;
                             vec4 newOccluderTexResult = mix(sceneColor, newCamColorTex, alpha);
                             vec4 newMattingTexResult = mix(sceneColor, newOccluderTexResult, showOccluder);
                             
                             /** LIQUID GLOW*/
                             vec2 p = (2.4*newTex.xy)/4.0;
                             for (int i = 1; i < 7; i++){
                                 vec2 newp = p + u_time*.01;
                                 newp.x+=2.1/float(i)*sin(float(i)*p.y+u_time/fluid_speed+0.3*float(i)) + .04;
                                 newp.y+=1.4/float(i)*sin(float(i)*p.x+u_time/fluid_speed+0.3*float(i+10)) - 0.05; // - mouse.x/mouse_factor+mouse_offset;
                                 p=newp;
                             }
                             
                             // new color on default camera image
                             vec4 newCol = vec4(.7*sin(3.0*p.x) + .7, .7*sin(3.0*p.y)+ .7, .7*sin(p.x+p.y)+.7, sceneColor.a);
                             // take newCol and matte it
                             vec4 newCameraColor = newCol;
                             
                             //vec4 pixelWithLiquid = texture2D(newCol.xy, newCol.xy);
                             vec4 newOccluderResult = mix(sceneColor, newCameraColor, alpha);
                             vec4 newMattingResult = mix(sceneColor, newOccluderResult, alpha);
                            
                             vec4 testtest = newTex*newCameraColor;
                             vec4 testColorSort = testtest*sceneColorSort;
                             vec4 pixelColorOccluderResult = mix(sceneColor, testtest, alpha);
                             
                             // camera Color is a sine of the actual color * time
                             vec4 cameraColor = vec4(sceneColor.r + abs(sin(u_time)), sceneColor.g + abs(cos(u_time)), sceneColor.b, sceneColor.a) * dilatedDepth;
                             vec4 occluderResult = mix(sceneColor, cameraColor, alpha);
                             vec4 mattingResult = mix(sceneColor, occluderResult, showOccluder);
                             gl_FragColor = pixelColorOccluderResult;
//                             gl_FragColor = occluderSort;

                         }
                );

                #endif
                    
    };
}}

#endif /* Camera_h */
