//==============================================================================
/*
    \author    Teresa, Adrian
*/
//==============================================================================

//------------------------------------------------------------------------------
#include "chai3d.h"
#include "MyProxyAlgorithm.h"
#include "MyMaterial.h"
//------------------------------------------------------------------------------
#include <GLFW/glfw3.h>
//------------------------------------------------------------------------------
using namespace chai3d;
using namespace std;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;
bool fullscreen = false;
bool mirroredDisplay = false;


//------------------------------------------------------------------------------
// DECLARED VARIABLES
//------------------------------------------------------------------------------

cWorld* world;
cCamera* camera;
cSpotLight* light;
cHapticDeviceHandler* handler;
cGenericHapticDevicePtr hapticDevice;

cLabel* labelRates;
cLabel* labelDebug;

cToolCursor* tool;

MyProxyAlgorithm* proxyAlgorithm;

bool simulationRunning = false;
bool simulationFinished = false;

cFrequencyCounter freqCounterGraphics;
cFrequencyCounter freqCounterHaptics;

cThread* hapticsThread;

GLFWwindow* window = NULL;
int width  = 0;
int height = 0;
int swapInterval = 1;
int texIndex = -1;

cVector3d workspaceOffset(0.0, 0.0, 0.0);
cVector3d cameraLookAt(0.0, 0.0, 0.0);
/*
cVector3d camPos = cVector3d(0.0, 0.0, 0.0);
cVector3d camDir = cVector3d(0.0, 0.0, 0.0);
cVector3d xAxis = cVector3d(0.0001, 0.0, 0.0);
cVector3d yAxis = cVector3d(0.0, 0.0001, 0.0);
cVector3d zAxis = cVector3d(0.0, 0.0, 0.0001);
*/
cMultiMesh* object;

//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height);
void errorCallback(int error, const char* a_description);
void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods);
void updateGraphics(void);
void updateHaptics(void);
void close(void);

//==============================================================================
/*
    TEMPLATE:    application.cpp

    Description of your application.
*/
//==============================================================================

int main(int argc, char* argv[])
{
    //--------------------------------------------------------------------------
    // INITIALIZATION
    //--------------------------------------------------------------------------

    cout << endl;
    cout << "-----------------------------------" << endl;
    cout << "CHAI3D" << endl;
    cout << "-----------------------------------" << endl << endl << endl;
    cout << "Keyboard Options:" << endl << endl;
    cout << "[f] - Enable/Disable full screen mode" << endl;
    cout << "[m] - Enable/Disable vertical mirroring" << endl;

    cout << "[+] (On Falcon) - Apply next texture to tool" << endl;
    cout << "[-] (On Falcon) - Apply previous texture to tool" << endl;
    cout << "[swirl] (On Falcon) - Disable texture on tool" << endl;

    cout << "[q] - Exit application" << endl;
    cout << endl << endl;

    //--------------------------------------------------------------------------
    // OPENGL - WINDOW DISPLAY
    //--------------------------------------------------------------------------

    if (!glfwInit())
    {
        cout << "failed initialization" << endl;
        cSleepMs(1000);
        return 1;
    }

    glfwSetErrorCallback(errorCallback);

    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int w = 0.8 * mode->height;
    int h = 0.5 * mode->height;
    int x = 0.5 * (mode->width - w);
    int y = 0.5 * (mode->height - h);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    if (stereoMode == C_STEREO_ACTIVE)
    {
        glfwWindowHint(GLFW_STEREO, GL_TRUE);
    }
    else
    {
        glfwWindowHint(GLFW_STEREO, GL_FALSE);
    }

    window = glfwCreateWindow(w, h, "Force Shading & Haptic Textures (Teresa Van)", NULL, NULL);
    if (!window)
    {
        cout << "failed to create window" << endl;
        cSleepMs(1000);
        glfwTerminate();
        return 1;
    }

    glfwGetWindowSize(window, &width, &height);
    glfwSetWindowPos(window, x, y);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(swapInterval);

#ifdef GLEW_VERSION
    if (glewInit() != GLEW_OK)
    {
        cout << "failed to initialize GLEW library" << endl;
        glfwTerminate();
        return 1;
    }
#endif


    //--------------------------------------------------------------------------
    // WORLD - CAMERA - LIGHTING
    //--------------------------------------------------------------------------

    // create a new world.
    world = new cWorld();
    world->m_backgroundColor.setBlack();

    camera = new cCamera(world);
    world->addChild(camera);

    camera->set( cVector3d (0.1, 0.0, 0.0),    // camera position (eye)
                 cameraLookAt,    // look at position (target)
                 cVector3d (0.0, 0.0, 1.0));   // direction of the (up) vector
    camera->setClippingPlanes(0.01, 1.0);
    camera->setStereoMode(stereoMode);
    camera->setStereoEyeSeparation(0.01);
    camera->setStereoFocalLength(0.5);
    camera->setMirrorVertical(mirroredDisplay);

    light = new cSpotLight(world);
    world->addChild(light);

    light->setEnabled(true);
    light->setLocalPos(0.7, 0.3, 1.0);
    light->setDir(-0.5,-0.2,-0.8);
    light->setShadowMapEnabled(true);
    light->m_shadowMap->setQualityHigh();
    light->setCutOffAngleDeg(10);

    double toolRadius = 0.0;

    //--------------------------------------------------------------------------
    // [CPSC.86] TEXTURED OBJECTS
    //--------------------------------------------------------------------------

    object = new cMultiMesh();

    object->loadFromFile("models/bomb-rounded.obj");
    object->createAABBCollisionDetector(toolRadius);
    object->computeBTN();
//	object->rotateAboutLocalAxisDeg(cVector3d(0,1,0), 90);

    cMesh* mesh = object->getMesh(0);

    mesh->m_material = MyMaterial::create();
    mesh->m_material->setWhiteAzure();
    mesh->m_material->setUseHapticShading(true);
    object->setStiffness(2000.0, true);

	MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(mesh->m_material);

    cTexture2dPtr albedoMap = cTexture2d::create();
    albedoMap->loadFromFile("textures/rust.png");
    albedoMap->setWrapModeS(GL_REPEAT);
    albedoMap->setWrapModeT(GL_REPEAT);
    albedoMap->setUseMipmaps(true);

    cTexture2dPtr heightMap = cTexture2d::create();
    heightMap->loadFromFile("textures/rust-height.png");
    heightMap->setWrapModeS(GL_REPEAT);
    heightMap->setWrapModeT(GL_REPEAT);
    heightMap->setUseMipmaps(true);

    cTexture2dPtr roughnessMap = cTexture2d::create();
    roughnessMap->loadFromFile("textures/rust-roughness.png");
    roughnessMap->setWrapModeS(GL_REPEAT);
    roughnessMap->setWrapModeT(GL_REPEAT);
    roughnessMap->setUseMipmaps(true);

    mesh->m_texture = albedoMap;
    material->m_height_map = heightMap;
    material->m_roughness_map = roughnessMap;
    material->hasTexture = true;

    mesh->setUseTexture(true);

	cMultiMesh* panal0 = new cMultiMesh();
	panal0->loadFromFile("models/tray.obj");
	panal0->createAABBCollisionDetector(toolRadius);
	panal0->computeBTN();
	
//	panal0->scale(0.25);
	panal0->rotateAboutLocalAxisDeg(cVector3d(0,1,0), 90);
	

	cMesh* pmesh0 = panal0->getMesh(0);
	pmesh0->m_material = MyMaterial::create();
	pmesh0->m_material->setWhiteAzure();
	pmesh0->m_material->setUseHapticShading(true);
	panal0->setStiffness(2000.0, true);
	MyMaterialPtr pmat0 = std::dynamic_pointer_cast<MyMaterial>(pmesh0->m_material);
	pmesh0->m_texture = albedoMap;
    pmat0->m_height_map = heightMap;
    pmat0->m_roughness_map = roughnessMap;
    pmat0->hasTexture = true;

    mesh->setUseTexture(true);

object->addChild(panal0);
    world->addChild(object);
	
//	world->addChild(panal0);
	

    //--------------------------------------------------------------------------
    // HAPTIC DEVICE
    //--------------------------------------------------------------------------

    handler = new cHapticDeviceHandler();
    handler->getDevice(hapticDevice, 0);

    hapticDevice->setEnableGripperUserSwitch(true);

    tool = new cToolCursor(world);
    world->addChild(tool);

    // [CPSC.86] replace the tool's proxy rendering algorithm with our own
    proxyAlgorithm = new MyProxyAlgorithm;
    delete tool->m_hapticPoint->m_algorithmFingerProxy;
    tool->m_hapticPoint->m_algorithmFingerProxy = proxyAlgorithm;
    tool->m_hapticPoint->m_sphereProxy->m_material->setWhite();
    
    tool->setRadius(0.001, toolRadius);
    tool->setHapticDevice(hapticDevice);
    tool->setWaitForSmallForce(true);
    tool->start();

    //--------------------------------------------------------------------------
    // WIDGETS
    //--------------------------------------------------------------------------

    cFontPtr font = NEW_CFONTCALIBRI20();

    labelRates = new cLabel(font);
    labelRates->m_fontColor.setWhite();
    camera->m_frontLayer->addChild(labelRates);

    labelDebug = new cLabel(font);
    labelDebug->m_fontColor.setWhite();
    camera->m_frontLayer->addChild(labelDebug);

    camera->setWireMode(true);
    //--------------------------------------------------------------------------
    // START SIMULATION
    //--------------------------------------------------------------------------

    hapticsThread = new cThread();
    hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

    atexit(close);

    //--------------------------------------------------------------------------
    // MAIN GRAPHIC LOOP
    //--------------------------------------------------------------------------

    windowSizeCallback(window, width, height);

    while (!glfwWindowShouldClose(window))
    {
        glfwGetWindowSize(window, &width, &height);
        updateGraphics();
        glfwSwapBuffers(window);
        glfwPollEvents();

        freqCounterGraphics.signal(1);
    }

    glfwDestroyWindow(window);

    glfwTerminate();

    // exit
    return 0;
}

//------------------------------------------------------------------------------

void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height)
{
    width  = a_width;
    height = a_height;
}

//------------------------------------------------------------------------------

void errorCallback(int a_error, const char* a_description)
{
    cout << "Error: " << a_description << endl;
}

//------------------------------------------------------------------------------

void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods)
{
    if (a_action != GLFW_PRESS)
        return;

    else if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q))
        glfwSetWindowShouldClose(a_window, GLFW_TRUE);

    else if (a_key == GLFW_KEY_F)
    {
        fullscreen = !fullscreen;
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        if (fullscreen)
        {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
        else
        {
            int w = 0.8 * mode->height;
            int h = 0.5 * mode->height;
            int x = 0.5 * (mode->width - w);
            int y = 0.5 * (mode->height - h);
            glfwSetWindowMonitor(window, NULL, x, y, w, h, mode->refreshRate);
            glfwSwapInterval(swapInterval);
        }
    }
    else if (a_key == GLFW_KEY_M)
    {
        mirroredDisplay = !mirroredDisplay;
        camera->setMirrorVertical(mirroredDisplay);
    }
}

//------------------------------------------------------------------------------

void close(void)
{
    simulationRunning = false;

    while (!simulationFinished) { cSleepMs(100); }

    hapticDevice->close();

    delete hapticsThread;
    delete world;
    delete handler;
}

//------------------------------------------------------------------------------

void updateGraphics(void)
{
    /////////////////////////////////////////////////////////////////////
    // UPDATE WIDGETS
    /////////////////////////////////////////////////////////////////////

    labelRates->setText(cStr(freqCounterGraphics.getFrequency(), 0) + " Hz / " +
        cStr(freqCounterHaptics.getFrequency(), 0) + " Hz");

    labelRates->setLocalPos((int)(0.5 * (width - labelRates->getWidth())), 15);


    labelDebug->setText(cStr(proxyAlgorithm->m_debugValue) +
                        ", " + cStr(proxyAlgorithm->m_debugVector.x()) +
                        ", " + cStr(proxyAlgorithm->m_debugVector.y()) +
                        ", " + cStr(proxyAlgorithm->m_debugVector.z())
                    );

    labelDebug->setLocalPos((int)(0.5 * (width - labelDebug->getWidth())), 40);


    /////////////////////////////////////////////////////////////////////
    // RENDER SCENE
    /////////////////////////////////////////////////////////////////////
    world->updateShadowMaps(false, mirroredDisplay);
    camera->renderView(width, height);

    glFinish();

    GLenum err;
    err = glGetError();
    if (err != GL_NO_ERROR) cout << "Error:  %s\n" << gluErrorString(err);
}

//------------------------------------------------------------------------------
void updateWorkspace(cVector3d devicePosition)
{
    if (devicePosition.x() < -0.025)
        workspaceOffset = cVector3d(-0.000075, 0.0, 0.0);
    else if (devicePosition.x() > 0.035)
        workspaceOffset = cVector3d(0.000075, 0.0, 0.0);
    else if (devicePosition.y() < -0.035)
        workspaceOffset = cVector3d(0.0, -0.000075, 0.0);
    else if (devicePosition.y() > 0.035)
        workspaceOffset = cVector3d(0.0, 0.000075, 0.0);
    else
        workspaceOffset = cVector3d(0.0, 0.0, 0.0);

    cVector3d toolPosition = tool->getLocalPos();
    cameraLookAt += workspaceOffset;
    toolPosition += workspaceOffset;

    tool->setLocalPos(toolPosition);
    camera->set( camera->getLocalPos() + workspaceOffset,    // camera position (eye)
                cameraLookAt,    // look at position (target)
                cVector3d (0.0, 0.0, 1.0));   // direction of the (up) vector
}

void updateHaptics(void)
{
    simulationRunning  = true;
    simulationFinished = false;
/*    
	cVector3d nextCamPos = camPos;
	cVector3d nextCamDir = camDir;
	double xoffset = 0;
	double yoffset = 0;
*/

    bool leftPressed = false;
    bool rightPressed = false;
    bool midPressed = false;

    while(simulationRunning)
    {
        /////////////////////////////////////////////////////////////////////
        // READ HAPTIC DEVICE
        /////////////////////////////////////////////////////////////////////

        cVector3d position;
        hapticDevice->getPosition(position);
        // read orientation
        cMatrix3d rotation;
        hapticDevice->getRotation(rotation);

        bool middle = false;
        bool left = false;
        bool right = false;

        hapticDevice->getUserSwitch(0, middle);
        hapticDevice->getUserSwitch(1, left);
        hapticDevice->getUserSwitch(3, right);
        
        if (middle) midPressed = true;
        if (left) leftPressed = true;
        if (right) rightPressed = true;

        if (midPressed)// && !middle)
        {
			midPressed = false;
        }
        if (leftPressed)// && !left)
        {
			object->rotateAboutLocalAxisDeg(cVector3d(0,0,1), 0.25);
			leftPressed = false;
        }
        if (rightPressed)// && !right)
        {
			object->rotateAboutLocalAxisDeg(cVector3d(0,0,-1), 0.25);
			rightPressed = false;
        }

        world->computeGlobalPositions();

        /////////////////////////////////////////////////////////////////////
        // UPDATE 3D CURSOR MODEL
        /////////////////////////////////////////////////////////////////////

        tool->updateFromDevice();

        /////////////////////////////////////////////////////////////////////
        // COMPUTE FORCES
        /////////////////////////////////////////////////////////////////////

        tool->computeInteractionForces();

        cVector3d force(0, 0, 0);
        cVector3d torque(0, 0, 0);
        double gripperForce = 0.0;

        tool->applyToDevice();

        /////////////////////////////////////////////////////////////////////
        // APPLY FORCES
        /////////////////////////////////////////////////////////////////////
/*		cVector3d toolPos = tool->getLocalPos();
		if ((position.x() > 0.02 && xoffset < 0.1) && (position.y() > 0.02 && yoffset < 0.1)) {
			nextCamPos = nextCamPos + xAxis + yAxis;
			nextCamDir = nextCamDir + xAxis + yAxis;
			tool->setLocalPos(toolPos.x() + 0.0001, toolPos.y() + 0.0001, toolPos.z());
			xoffset += 0.0001;
			yoffset += 0.0001;
		}
		else if ((position.x() < -0.02 && xoffset > -0.1) && (position.y() < -0.02 && yoffset > -0.1)) {
			nextCamPos = nextCamPos - xAxis - yAxis;
			nextCamDir = nextCamDir - xAxis - yAxis;
			tool->setLocalPos(toolPos.x() - 0.0001, toolPos.y() - 0.0001, toolPos.z());
			xoffset -= 0.0001;
			yoffset -= 0.0001;
		}
		else if ((position.x() > 0.02 && xoffset < 0.1) && (position.y() < -0.02 && yoffset > -0.1)) {
			nextCamPos = nextCamPos + xAxis - yAxis;
			nextCamDir = nextCamDir + xAxis - yAxis;
			tool->setLocalPos(toolPos.x() + 0.0001, toolPos.y() - 0.0001, toolPos.z());
			xoffset += 0.0001;
			yoffset -= 0.0001;
		}
		else if ((position.x() < -0.02 && xoffset > -0.1) && (position.y() > 0.02 && yoffset < 0.1)) {
			nextCamPos = nextCamPos - xAxis + yAxis;
			nextCamDir = nextCamDir - xAxis + yAxis;
			tool->setLocalPos(toolPos.x() - 0.0001, toolPos.y() + 0.0001, toolPos.z());
			xoffset -= 0.0001;
			yoffset += 0.0001;
		}
		else if (position.x() > 0.02 && xoffset < 0.1) {
			nextCamPos = nextCamPos + xAxis;
			nextCamDir = nextCamDir + xAxis;
			tool->setLocalPos(toolPos.x() + 0.0001, toolPos.y(), toolPos.z());
			xoffset += 0.0001;
		}
		else if (position.x() < -0.02 && xoffset > -0.1) {
			nextCamPos = nextCamPos - xAxis;
			nextCamDir = nextCamDir - xAxis;
			tool->setLocalPos(toolPos.x() - 0.0001, toolPos.y(), toolPos.z());
			xoffset -= 0.0001;
		}
		else if (position.y() > 0.02 && yoffset < 0.1) {
			nextCamPos = nextCamPos + yAxis;
			nextCamDir = nextCamDir + yAxis;
			tool->setLocalPos(toolPos.x(), toolPos.y() + 0.0001, toolPos.z());
			yoffset += 0.0001;
		}
		else if (position.y() < -0.02 && yoffset > -0.1) {
			nextCamPos = nextCamPos - yAxis;
			nextCamDir = nextCamDir - yAxis;
			tool->setLocalPos(toolPos.x(), toolPos.y() - 0.0001, toolPos.z());
			yoffset -= 0.0001;
		}
				
		camera->set(nextCamPos, nextCamDir, cVector3d(0,0,1));
*/
        updateWorkspace(position);

        freqCounterHaptics.signal(1);
    }

    simulationFinished = true;
}

//------------------------------------------------------------------------------
