//
//  Camera.cpp
//  example-metalcam
//
//  Created by Joseph Chow on 7/26/18.
//

#include <stdio.h>
#include "Camera.h"

using namespace ofxARKit::common;

namespace ofxARKit {
    namespace core {
        
        Camera::Camera(ARSession * session):debugMatteShaderBuilt(false){
            
   
            
            this->session = session;
            viewport = CGRectMake(0,0,ofGetWindowWidth(),ofGetWindowHeight());
            auto context = ofxiOSGetGLView().context;
            
            setup(session,viewport,context);
            
            mesh = ofMesh::plane(ofGetWindowWidth(), ofGetWindowHeight());

 if(this->session.configuration.frameSemantics == ARFrameSemanticPersonSegmentationWithDepth){
                
                shader.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
                shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
            }else{

                shader.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
                shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
            }
            
            shader.linkProgram();
            
            near = 0.1f;
            far = 1000.0f;
            
            
        }
        
        CVOpenGLESTextureRef Camera::getTexture(){
            
            // remember - you'll need to flip the uv on the y-axis to get the correctly oriented image.
            return [_view getConvertedTexture];
        }
    
        //======== MATTE API ============ //
#if defined( __IPHONE_13_0 )
                    
        CVOpenGLESTextureRef Camera::getTextureMatteAlpha(){
            return [_view getConvertedTextureMatteAlpha];
        }
        CVOpenGLESTextureRef Camera::getTextureMatteDepth(){
            return [_view getConvertedTextureMatteDepth];
        }
        CVOpenGLESTextureRef Camera::getTextureDepth(){
            return [_view getConvertedTextureDepth];
        }
        ofMatrix3x3 Camera::getAffineTransform(){
            
            // correspondance CGAffineTransform --> ofMatrix3x3 :
            //                    a  b  0       |     a  b  c
            //                    c  d  0       |     d  e  f
            //                    tx ty 1       |     g  h  i
            
            CGAffineTransform cAffine = [_view getAffineCameraTransform];
            ofMatrix3x3 matTransAffine;
            matTransAffine.a = cAffine.a;
            matTransAffine.b = cAffine.b;
            matTransAffine.d = cAffine.c;
            matTransAffine.e = cAffine.d;
            matTransAffine.g = cAffine.tx;
            matTransAffine.h = cAffine.ty;
            
            return matTransAffine;
        }
#endif
        
        void Camera::update(){
            [_view draw];
            
            cameraMatrices.cameraTransform = common::convert<matrix_float4x4,ofMatrix4x4>(session.currentFrame.camera.transform);
            
            getMatricesForOrientation(orientation, near, far);
        }


        ofxARKit::common::ARCameraMatrices Camera::getCameraMatrices(){
            return cameraMatrices;
        }


        common::ARCameraMatrices Camera::getMatricesForOrientation(UIInterfaceOrientation orientation,float near, float far){
            
            cameraMatrices.cameraView = toMat4([session.currentFrame.camera viewMatrixForOrientation:orientation]);
            cameraMatrices.cameraProjection = toMat4([session.currentFrame.camera projectionMatrixForOrientation:orientation viewportSize:viewport.size zNear:(CGFloat)near zFar:(CGFloat)far]);
            
            return cameraMatrices;
        }
        
        ARTrackingStateReason Camera::getTrackingState(){
            return session.currentFrame.camera.trackingStateReason;
        }
        
        void Camera::logTrackingState(){
            
            if(debugMode){
                trackingStateReason = session.currentFrame.camera.trackingStateReason;
                

                switch(trackingStateReason){
                    case ARTrackingStateReasonNone:
                        ofLog(OF_LOG_NOTICE,"Tracking state is a-ok!");
                        break;
                        
                    case ARTrackingStateReasonInitializing:
                        ofLog(OF_LOG_NOTICE,"Tracking is warming up and waiting for enough information to start tracking");
                        break;
                        
                    case ARTrackingStateReasonExcessiveMotion:
                        ofLog(OF_LOG_ERROR,"There is excessive motion at the moment, tracking is affected.");
                        break;
                        
                    case ARTrackingStateReasonInsufficientFeatures:
                        ofLog(OF_LOG_ERROR,"There are not enough features found to enable tracking");
                        break;
                    case ARTrackingStateReasonRelocalizing:
                        
                        break;
                }
            }
        }
        void Camera::setARCameraMatrices(){
            ofSetMatrixMode(OF_MATRIX_PROJECTION);
            ofLoadMatrix(cameraMatrices.cameraProjection);
            ofSetMatrixMode(OF_MATRIX_MODELVIEW);
            ofLoadMatrix(cameraMatrices.cameraView);
        }
        
        void Camera::updateInterfaceOrientation(int newOrientation){
            orientation = (UIInterfaceOrientation)newOrientation;
          
            auto width = ofGetWindowWidth();
            auto height = ofGetWindowHeight();
            
            // this might be an oF thing - but values seem to be reversed after the first call when calling ofGetWindowWidth
            switch(orientation){
                case UIInterfaceOrientationPortrait:
                    
                    if(width > height){
                        viewport = CGRectMake(0,0,ofGetWindowHeight(),ofGetWindowWidth());
                    }
                    break;
                    
                case UIInterfaceOrientationLandscapeLeft:
                    
                    if(width < height){
                        viewport = CGRectMake(0,0,ofGetWindowHeight(),ofGetWindowWidth());
                    }
                    break;
                    
                case UIInterfaceOrientationLandscapeRight:
                    
                    if(width < height){
                        viewport = CGRectMake(0,0,ofGetWindowHeight(),ofGetWindowWidth());
                    }
                    break;
                    
                case UIInterfaceOrientationUnknown:
                    if(width > height){
                        viewport = CGRectMake(0,0,ofGetWindowHeight(),ofGetWindowWidth());
                    }
                    break;
                    
                case UIInterfaceOrientationPortraitUpsideDown:
                    if(width > height){
                        viewport = CGRectMake(0,0,ofGetWindowHeight(),ofGetWindowWidth());
                    }
                    break;
            }
        }
        
        void Camera::draw(){
            // get and draw texture
            auto _tex = [_view getConvertedTexture];
                        
            if(_tex){
                shader.begin();
                shader.setUniformTexture("tex", CVOpenGLESTextureGetTarget(_tex), CVOpenGLESTextureGetName(_tex), 0);
                mesh.draw();
                shader.end();
            }
        }
    
        /**WORK HERE **/
        void Camera::drawDebugPersonSegmentation(){

            if(!debugMatteShaderBuilt){
                shader = getDefaultMatteShader();
                debugMatteShaderBuilt = true;
            }

            // get and draw texture
            auto _tex = [_view getConvertedTexture];
            auto _texMatteAlpha = [_view getConvertedTextureMatteAlpha];
            auto _texMatteDepth = [_view getConvertedTextureMatteDepth];
            auto _texDepth = [_view getConvertedTextureDepth];
            
            CGAffineTransform cAffine = [_view getAffineCameraTransform];
            
            /*
            const std::size_t MAX_BUFFER_SIZE = 45;
            if (testVector.size() < MAX_BUFFER_SIZE) {
                testVector.push_back(_tex);
                textMatteAlphaVector.push_back(_texMatteAlpha);
                textMatteDepthVector.push_back(_texMatteDepth);
                nextIndexToWrite = testVector.size();
            } else {
                testVector[nextIndexToWrite] = _tex;
                textMatteAlphaVector[nextIndexToWrite] = _texMatteAlpha;
                textMatteDepthVector[nextIndexToWrite] = _texMatteDepth;
            }
            nextIndexToWrite = (nextIndexToWrite + 1) % MAX_BUFFER_SIZE;
            */
            
            
            if(_tex){
                /*
                for (int i = 0; i < testVector.size(); i++) {
                    shader.begin();
                    shader.setUniformTexture("tex", CVOpenGLESTextureGetTarget(testVector[i]), CVOpenGLESTextureGetName(testVector[i]), 0);
                    
                    if(this->session.configuration.frameSemantics == ARFrameSemanticPersonSegmentationWithDepth){
                         if(_texMatteAlpha)shader.setUniformTexture("texAlphaBody", CVOpenGLESTextureGetTarget(textMatteAlphaVector[i]), CVOpenGLESTextureGetName(textMatteAlphaVector[i]), 1);
                         if(_texMatteDepth)shader.setUniformTexture("texDepthBody", CVOpenGLESTextureGetTarget(textMatteDepthVector[i]), CVOpenGLESTextureGetName(textMatteDepthVector[i]), 2);
                         if(_texDepth)shader.setUniformTexture("texDepth", CVOpenGLESTextureGetTarget(_texDepth), CVOpenGLESTextureGetName(_texDepth), 3);
                         
                         // textures affine coordinates
                         shader.setUniform4f("cAffineCamABCD", float(cAffine.a), float(cAffine.b), float(cAffine.c), float(cAffine.d));
                         shader.setUniform2f("cAffineCamTxTy", float(cAffine.tx), float(cAffine.ty));

                         shader.setUniform1f("u_time", ofGetElapsedTimef());
                         shader.setUniformMatrix4f("u_CameraProjectionMat", getProjectionMatrix());
                     }
                     mesh.draw();
                     shader.end();
                 */
                
                shader.begin();
                shader.setUniformTexture("tex", CVOpenGLESTextureGetTarget(_tex), CVOpenGLESTextureGetName(_tex), 0);
                
                if(this->session.configuration.frameSemantics == ARFrameSemanticPersonSegmentationWithDepth){
                    if(_texMatteAlpha)shader.setUniformTexture("texAlphaBody", CVOpenGLESTextureGetTarget(_texMatteAlpha), CVOpenGLESTextureGetName(_texMatteAlpha), 1);
                    if(_texMatteDepth)shader.setUniformTexture("texDepthBody", CVOpenGLESTextureGetTarget(_texMatteDepth), CVOpenGLESTextureGetName(_texMatteDepth), 2);
                    if(_texDepth)shader.setUniformTexture("texDepth", CVOpenGLESTextureGetTarget(_texDepth), CVOpenGLESTextureGetName(_texDepth), 3);
                    
                    // textures affine coordinates
                    shader.setUniform4f("cAffineCamABCD", float(cAffine.a), float(cAffine.b), float(cAffine.c), float(cAffine.d));
                    shader.setUniform2f("cAffineCamTxTy", float(cAffine.tx), float(cAffine.ty));

                    shader.setUniform1f("u_time", ofGetElapsedTimef());
                    shader.setUniformMatrix4f("u_CameraProjectionMat", getProjectionMatrix());
                }
                mesh.draw();
                shader.end();
            }
        }

        
        glm::mat4 Camera::getProjectionMatrix(){
            return convert<ofMatrix4x4, glm::mat4>(cameraMatrices.cameraProjection);
        }
        glm::mat4 Camera::getViewMatrix(){
            return convert<ofMatrix4x4, glm::mat4>(cameraMatrices.cameraView);
        }
        
        glm::mat4 Camera::getTransformMatrix(){
             return convert<ofMatrix4x4, glm::mat4>(cameraMatrices.cameraTransform);
        }
    }
}
