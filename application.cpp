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

double toolRadius = 0.0005;

cMultiMesh* bomb;
cMesh * deathScreen;
cMesh * background;
cMesh * table;
vector<cMultiMesh*> panels;
bool gameOver = false;

///////////////////////////////////////////////////

int timeLimit[4] = {1,0,1,0}; // mm:ss
vector<cTexture2dPtr> numberTextures;
const string numberTextureFiles[11] = 
{
    "0.png",
    "1.png",
    "2.png",
    "3.png",
    "4.png",
    "5.png",
    "6.png",
    "7.png",
    "8.png",
    "9.png",
    "dots.png",
};

vector<cMesh*> timerNumbers;
double startTime;

///////////////////////////////////////////////////

int brailleOrder[4] = {3, 1, 2, 0};
vector<cTexture2dPtr> brailleTextures;
vector<cTexture2dPtr> brailleHeights;
const string brailleTextureFiles[4] = 
{
	"brailleG.png",
	"brailleR.png",
	"brailleW.png",
	"brailleY.png"
};
vector<cMesh*> brailleLetters;

///////////////////////////////////////////////////

vector<cMultiMesh*> wires;
const string wireModels[4] = 
{
    "wire1.obj",
    "wire2.obj",
    "wire3.obj",
    "wire4.obj",
};

cColorf color1;
cColorf color2;
cColorf color3;
cColorf color4;
vector<cColorf> wireColors = 
{
    color1,
    color2,
    color3,
    color4,
};

//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

void windowSizeCallback(GLFWwindow* a_window, int a_width, int a_height);
void errorCallback(int error, const char* a_description);
void keyCallback(GLFWwindow* a_window, int a_key, int a_scancode, int a_action, int a_mods);
void updateGraphics(void);
void updateHaptics(void);
void close(void);

void SetColors()
{
    wireColors[0].setRed();
    wireColors[1].setYellow();
    wireColors[2].setGreen();
    wireColors[3].setWhite();
}

void RemoveWorld()
{
    world->removeChild(bomb);
    world->removeChild(table);
    world->removeChild(background);
}

void CreateBrailleOrder() {
	srand((unsigned)time(0));
	int randomInteger;
//	int lowest = 0;
//	int highest = 3;
//	int range = (highest - lowest);
	vector<int> tempOrder;
	while (tempOrder.size() < 4) {
		randomInteger = (int)(((float)rand()/(float)(RAND_MAX))*10);
		if (randomInteger < 4 && find(tempOrder.begin(), tempOrder.end(), randomInteger) == tempOrder.end())
			tempOrder.push_back(randomInteger);
	}
	for (int i=0; i<4; i++)
		brailleOrder[i] = tempOrder[i];

//	for (int i=0; i<tempOrder.size(); i++) {
//		std::cout << tempOrder[i] <<std::endl;
//	}
	
}

void CreateNumberTextures()
{
    for (string s : numberTextureFiles)
    {
        cTexture2dPtr tex = cTexture2d::create();
        tex->loadFromFile("textures/" + s);
        tex->setWrapModeS(GL_REPEAT);
        tex->setWrapModeT(GL_REPEAT);
        tex->setUseMipmaps(true);
        numberTextures.push_back(tex);
    }
}

void CreateBrailleTextures() {
//    for (string s : brailleTextureFiles)
    for (int i=0; i<4; i++)
    {
        cTexture2dPtr tex = cTexture2d::create();
        tex->loadFromFile("textures/" + brailleTextureFiles[brailleOrder[i]]);
        tex->setWrapModeS(GL_REPEAT);
        tex->setWrapModeT(GL_REPEAT);
        tex->setUseMipmaps(true);
        brailleHeights.push_back(tex);
    }

}

void CreateDeathScreen()
{
    deathScreen = new cMesh();
    cCreatePlane(deathScreen, 0.15, 0.1, cVector3d(0, -0.001, 0.03));
    deathScreen->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
    deathScreen->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));

    cTexture2dPtr albedoMap = cTexture2d::create();
    albedoMap->loadFromFile("textures/deathscreen.png");
    albedoMap->setWrapModeS(GL_REPEAT);
    albedoMap->setWrapModeT(GL_REPEAT);
    albedoMap->setUseMipmaps(true);
    deathScreen->m_texture = albedoMap;
    deathScreen->setUseTexture(true);
}

void CreateEnvironment()
{
    background = new cMesh();
    cCreatePlane(background, 0.25, 0.25, cVector3d(0, 0, -0.1));
    background->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
    background->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));

    cTexture2dPtr albedoMap = cTexture2d::create();
    albedoMap->loadFromFile("textures/background.jpg");
    albedoMap->setWrapModeS(GL_REPEAT);
    albedoMap->setWrapModeT(GL_REPEAT);
    albedoMap->setUseMipmaps(true);
    background->m_texture = albedoMap;
    background->setUseTexture(true);

    world->addChild(background);

    /////////////////////////////////////////////////////////////////////////////////

    table = new cMesh();
    cCreatePlane(table, 0.12, 0.075, cVector3d(0, 0, -0.025));
    table->createBruteForceCollisionDetector();
    table->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
    table->rotateAboutGlobalAxisDeg(cVector3d(0,0,1), 80);
    table->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));

    table->m_material = MyMaterial::create();
    table->m_material->setBrownWheat();
    table->m_material->setUseHapticShading(true);
    table->setStiffness(2000.0, true);

	MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(table->m_material);

    albedoMap = cTexture2d::create();
    albedoMap->loadFromFile("textures/bamboo-wood-albedo.png");
    albedoMap->setWrapModeS(GL_REPEAT);
    albedoMap->setWrapModeT(GL_REPEAT);
    albedoMap->setUseMipmaps(true);

    cTexture2dPtr heightMap = cTexture2d::create();
    heightMap->loadFromFile("textures/bamboo-wood-height.png");
    heightMap->setWrapModeS(GL_REPEAT);
    heightMap->setWrapModeT(GL_REPEAT);
    heightMap->setUseMipmaps(true);

    cTexture2dPtr roughnessMap = cTexture2d::create();
    roughnessMap->loadFromFile("textures/bamboo-wood-roughness.png");
    roughnessMap->setWrapModeS(GL_REPEAT);
    roughnessMap->setWrapModeT(GL_REPEAT);
    roughnessMap->setUseMipmaps(true);

    table->m_texture = albedoMap;
    material->m_height_map = heightMap;
    material->m_roughness_map = roughnessMap;
    material->hasTexture = true;
    table->setUseTexture(true);

    world->addChild(table);
}

void CreateBomb()
{
    bomb = new cMultiMesh();

    bomb->loadFromFile("models/bomb-rounded.obj");
    bomb->createAABBCollisionDetector(toolRadius);
    bomb->computeBTN();
    bomb->setLocalPos(cVector3d(0.005, 0, 0));

    cMesh* mesh = bomb->getMesh(0);

    mesh->m_material = MyMaterial::create();
    mesh->m_material->setWhite();
    mesh->m_material->setUseHapticShading(true);
    bomb->setStiffness(2000.0, true);

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

    world->addChild(bomb);
}

void CreateWires()
{
    cMultiMesh* base = new cMultiMesh();
    base->loadFromFile("models/wirebase.obj");
    base->createAABBCollisionDetector(toolRadius);
    base->computeBTN();

    cMesh* mesh = base->getMesh(0);
    mesh->m_material = MyMaterial::create();
    mesh->m_material->setColorf(0.2, 0.2, 0.2);
    mesh->m_material->setUseHapticShading(true);
    base->setStiffness(2000.0, true);

    MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(mesh->m_material);
    material->hasTexture = false;

    base->setLocalPos(cVector3d(0.0005, 0.001, 0.0005));
    base->rotateAboutGlobalAxisRad(cVector3d(0,0,-1), cDegToRad(90));
    panels[0]->addChild(base);

    double posX = -0.0035;
    double posY = 0.0005;
    double posZ = 0.0025;
    double spacing = 0.003;
    double offsetY = 0.0001;
    for (int i = 0; i < 4; i++)
    {
        cMultiMesh* wire = new cMultiMesh();
        wire->loadFromFile("models/" + wireModels[i]);
        wire->createAABBCollisionDetector(toolRadius);
        wire->computeBTN();

        cMesh* mesh = wire->getMesh(0);
        mesh->m_material = MyMaterial::create();
        mesh->m_material->setColor(wireColors[i]);
        mesh->m_material->setUseHapticShading(true);
        wire->setStiffness(2000.0, true);

        MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(mesh->m_material);
        material->hasTexture = false;

        wire->setLocalPos(cVector3d(posZ, posX + (i*spacing), posY - (i*offsetY)));
        wire->rotateAboutGlobalAxisRad(cVector3d(0,0,-1), cDegToRad(90));

        panels[0]->addChild(wire);
    }
}

// right pos : 0.019
// left pos : - 0.019
// top = 0.0095
// bottom = -0.0095

// front = 0.006
// behind: 0.0125

cVector3d panelPositions[11] = 
{
    cVector3d(0.006, -0.019, 0.0095),
    cVector3d(0.006, 0.0, 0.0095),
    cVector3d(0.006, 0.019, 0.0095),

    cVector3d(0.006, -0.019, -0.0095),
    cVector3d(0.006, 0.019, -0.0095),

    cVector3d(-0.006, -0.019, 0.0095),
    cVector3d(-0.006, 0.0, 0.0095),
    cVector3d(-0.006, 0.019, 0.0095),

    cVector3d(-0.006, -0.019, -0.0095),
    cVector3d(-0.006, 0.0, -0.0095),
    cVector3d(-0.006, 0.019, -0.0095),
};

void CreatePanels()
{
    for (int i = 0; i < 11; i++)
    {
        cMultiMesh* panel = new cMultiMesh();
        panel->loadFromFile("models/basepanel.obj");
        panel->createAABBCollisionDetector(toolRadius);
        panel->computeBTN();

        cMesh* mesh = panel->getMesh(0);
        mesh->m_material = MyMaterial::create();
        mesh->m_material->setWhiteLinen();
        mesh->m_material->setUseHapticShading(true);
        panel->setStiffness(2000.0, true);

        MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(mesh->m_material);
        material->hasTexture = false;

        bomb->addChild(panel);
        panels.push_back(panel);
    }
    
}

void SetPanelPositions()
{
    int i = 0;
    for (cMultiMesh * panel : panels)
    {
        panel->setLocalPos(panelPositions[i]);
        if (panel->getLocalPos().x() < 0)
            panel->rotateAboutGlobalAxisRad(cVector3d(0,0,1), cDegToRad(180));
        i++;
    }
}

void CreateBraillePuzzle()
{
    cTexture2dPtr btex = cTexture2d::create();
    btex->loadFromFile("textures/brailleEmpty.png");
    btex->setWrapModeS(GL_REPEAT);
    btex->setWrapModeT(GL_REPEAT);
    btex->setUseMipmaps(true);

    // Create timer number planes
    double spacing = 0.0058;
    double posX = -0.009;
    for (int i = 0; i < 4; i++)
    {
		/*
        cMesh * mesh = new cMesh();
        mesh->m_material = MyMaterial::create();
        cCreatePlane(mesh, spacing, 0.015, cVector3d((i<2) ? posX + i*spacing : -posX - (i-2)*spacing, 0.0, 0.005));
        mesh->m_material->setWhiteLinen();
//	    mesh->createBruteForceCollisionDetector();
        mesh->createAABBCollisionDetector(toolRadius);
		mesh->computeBTN();
        mesh->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
        mesh->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));
        mesh->scale(0.55);
        
		mesh->m_material->setWhite();
		mesh->m_material->setUseHapticShading(true);
        mesh->setStiffness(2000.0, true);
//        mesh->m_texture = btex;
        mesh->m_texture = brailleHeights[i];
        MyMaterialPtr mat = std::dynamic_pointer_cast<MyMaterial>(mesh->m_material);
        mat->m_height_map = brailleHeights[i];
        mat->hasTexture = true;
        mesh->setUseTexture(true);
        */
        
		cMesh * mesh = new cMesh();
		cCreatePlane(mesh, 0.006, 0.015, cVector3d(posX + i*spacing, 0.0, 0.00124));
        mesh->createBruteForceCollisionDetector();
        mesh->computeBTN();

        mesh->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
        mesh->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));
        mesh->scale(.55);
        mesh->setUseTransparency(true, true);
		
		mesh->m_material = MyMaterial::create();
		mesh->m_material->setWhite();
		mesh->m_material->setUseHapticShading(true);
		mesh->setStiffness(2000.0, true);
		mesh->setFriction(0, 0);

		MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(mesh->m_material);

		cTexture2dPtr albedoMap = cTexture2d::create();
	//    albedoMap->loadFromFile("textures/"+brailleTextureFiles[i]); //uncomment to see braille
		albedoMap->loadFromFile("textures/brailleEmpty.png");
		albedoMap->setWrapModeS(GL_REPEAT);
		albedoMap->setWrapModeT(GL_REPEAT);
		albedoMap->setUseMipmaps(true);

		cTexture2dPtr heightMap = cTexture2d::create();
		heightMap->loadFromFile("textures/"+brailleTextureFiles[brailleOrder[i]]);
		heightMap->setWrapModeS(GL_REPEAT);
		heightMap->setWrapModeT(GL_REPEAT);
		heightMap->setUseMipmaps(true);

		cTexture2dPtr roughnessMap = cTexture2d::create();
		roughnessMap->loadFromFile("textures/brailleRoughness.png");
		roughnessMap->setWrapModeS(GL_REPEAT);
		roughnessMap->setWrapModeT(GL_REPEAT);
		roughnessMap->setUseMipmaps(true);

		mesh->m_texture = albedoMap;
		material->m_height_map = heightMap;
		material->m_roughness_map = roughnessMap;
		material->hasTexture = true;

		mesh->setUseTexture(true);
        panels[4]->addChild(mesh);
        brailleLetters.push_back(mesh);
    }
}

void CreateBrailleLegend()
{
	cMesh * mesh = new cMesh();
	cCreatePlane(mesh,0.01,0.01,cVector3d(0.0125,0.0065,0.005));
	    mesh->createBruteForceCollisionDetector();
        mesh->rotateAboutGlobalAxisDeg(cVector3d(0,-1,0), 90);
        mesh->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));
        mesh->scale(1.5);
        mesh->setUseTransparency(true, true);
	
    mesh->m_material = MyMaterial::create();
    mesh->m_material->setWhite();
    mesh->m_material->setUseHapticShading(true);
    mesh->setStiffness(2000.0, true);

	MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(mesh->m_material);

    cTexture2dPtr albedoMap = cTexture2d::create();
    albedoMap->loadFromFile("textures/brailleTornLegend.png");
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
	
	bomb->addChild(mesh);
	
}

void CreateTimer()
{
    // Create timer object
    cMultiMesh * timer = new cMultiMesh();
    timer->loadFromFile("models/timer.obj");
    timer->createAABBCollisionDetector(toolRadius);
    timer->computeBTN();

    cMesh* mesh = timer->getMesh(0);

    mesh->m_material = MyMaterial::create();
    mesh->m_material->setWhiteLinen();
    mesh->m_material->setUseHapticShading(true);
    timer->setStiffness(2000.0, true);

	MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(mesh->m_material);
    material->hasTexture = false;

    // Create timer number planes
    double spacing = 0.0058;
    double posX = -0.01;
    for (int i = 0; i < 4; i++)
    {
        cMesh * mesh = new cMesh();
        cCreatePlane(mesh, spacing, 0.015, cVector3d((i<2) ? posX + i*spacing : -posX - (i-2)*spacing, 0.0, 0.0035));
        mesh->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
        mesh->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));
        mesh->scale(0.55);
        mesh->setUseTexture(true);
        timer->addChild(mesh);
        timerNumbers.push_back(mesh);
    }

    mesh = new cMesh();
    cCreatePlane(mesh, 0.00275, 0.015, cVector3d(0.0, 0.0, 0.0035));
    mesh->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
    mesh->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));
    mesh->scale(0.55);
    mesh->m_texture = numberTextures[10];
    mesh->setUseTexture(true);
    timer->addChild(mesh);

    cMesh * temp = timerNumbers[3];
    timerNumbers[3] = timerNumbers[2];
    timerNumbers[2] = temp;

    timer->setLocalPos(0.006, 0.0, -0.011);

    bomb->addChild(timer);
}

void UpdateTimer()
{
    if (timeLimit[0] < 0)
    {
        RemoveWorld();
        world->addChild(deathScreen);
        gameOver = true;
        return;
    }

    int i = 0;
    for (cMesh * m : timerNumbers)
    {
        m->m_texture = numberTextures[timeLimit[i]];
        i++;
    }
}

void UpdateTimeElapsed()
{
    if (timeLimit[3] == 0)
    {
        if (timeLimit[2] == 0)
        {
            if (timeLimit[1] == 0)
            {
                timeLimit[0] -= 1;
                timeLimit[1] = 9;
                timeLimit[2] = 5;
                timeLimit[3] = 9;
            }
            else
            {
                timeLimit[1] -= 1;
                timeLimit[2] = 5;
                timeLimit[3] = 9;
            }
        }
        else
        {
            timeLimit[2] -= 1;
            timeLimit[3] = 9;
        }
    }
    else
    {
        timeLimit[3] -= 1;
    }

    // cout << timeLimit[0] << ", " << timeLimit[1] << ", " << timeLimit[2] << ", " << timeLimit[3] << "\n";
}

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
    light->setLocalPos(0.7, 0.3, 1.5);
    light->setDir(-0.5,-0.2,-0.8);
    light->setShadowMapEnabled(true);
    light->m_shadowMap->setQualityHigh();
    light->setCutOffAngleDeg(10);

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
    
    tool->setRadius(0.0005, toolRadius);
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
    
    SetColors();
    CreateEnvironment();
    CreateBomb();
    CreatePanels();
    CreateWires();
    
    CreateNumberTextures();
    CreateTimer();
    UpdateTimer();

	CreateBrailleOrder();

	CreateBrailleTextures();
	CreateBraillePuzzle();  


    CreateBrailleLegend();
    CreateDeathScreen();

    SetPanelPositions();
    
    cPrecisionClock clock;
    startTime = clock.getCPUTimeSeconds();

    windowSizeCallback(window, width, height);

    while (!glfwWindowShouldClose(window))
    {
        if (!gameOver)
        {
            double previousTime = clock.getCPUTimeSeconds();
            double deltaTime = previousTime - startTime;
            // cout << deltaTime << "\n";
            if (deltaTime >= 1)
            {
                UpdateTimeElapsed();
                UpdateTimer();
                startTime = previousTime;
            }
        }

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

    if ((a_key == GLFW_KEY_ESCAPE) || (a_key == GLFW_KEY_Q) && a_action == GLFW_PRESS)  
        glfwSetWindowShouldClose(a_window, GLFW_TRUE);

    else if (a_key == GLFW_KEY_F && a_action == GLFW_PRESS)
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
    else if (a_key == GLFW_KEY_M && a_action == GLFW_PRESS)
    {
        mirroredDisplay = !mirroredDisplay;
        camera->setMirrorVertical(mirroredDisplay);
    }
    else if (a_key == GLFW_KEY_RIGHT)
    {
			bomb->rotateAboutLocalAxisDeg(cVector3d(0,0,1), cDegToRad(180));
    }
    else if (a_key == GLFW_KEY_LEFT)
    {
			bomb->rotateAboutLocalAxisDeg(cVector3d(0,0,-1), cDegToRad(180));
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
			bomb->rotateAboutLocalAxisDeg(cVector3d(0,0,-1), 0.25);
			leftPressed = false;
        }
        if (rightPressed)// && !right)
        {
			bomb->rotateAboutLocalAxisDeg(cVector3d(0,0,1), 0.25);
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

        // updateWorkspace(position);

        freqCounterHaptics.signal(1);
    }

    simulationFinished = true;
}

//------------------------------------------------------------------------------
