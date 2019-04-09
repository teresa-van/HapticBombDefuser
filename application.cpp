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
extern int wireID = 6;
extern cVector3d contactForce(0,0,0);
extern cVector3d returnForce(0,0,0);

struct particle;
struct spring;
struct pbutton;

struct particle {
	double radius;
	double mass;
	double damping;
	cVector3d position;
	cVector3d velocity;
	cVector3d sphereForce;
	bool fixed;
	vector<spring*> springs;
	cShapeSphere* sphere;
	cMesh* msphere;
};
struct spring {
	double k; // spring constant
	double dk; // damping constant
	double restlength;
	particle *p1;
	particle *p2;
	cShapeLine* line;
};
struct pbutton {
	vector<particle*> particles;
	vector<spring*> springs;
};

void makeParticle(particle *p, double r, double m, double d, cVector3d pos, bool fixed) {
	p->radius = r;
	p->mass = m;
	p->damping = d;
	p->position = pos;
	p->velocity = cVector3d(0,0,0);
	p->sphereForce = cVector3d(0,0,0);
	p->sphere = new cShapeSphere(r);
	p->sphere->setLocalPos(pos);
	p->fixed = fixed;
}
void makeSpring(spring *s, double k, double d, double len, particle *a, particle *b) {
	s->k = k;
	s->dk = d;
	s->restlength = len;
	s->p1 = a;
	s->p2 = b;
	s->line = new cShapeLine();
	s->line->m_pointA = s->p1->position;
	s->line->m_pointB = s->p2->position;
}

bool checkButton(pbutton *o, double dt, int i) {
//	cVector3d fdevice = cVector3d(0,0,0);
	bool pressed = false;
	for(particle *p : o->particles) {
		if (p->fixed) continue;
		
//		cVector3d fg = gravity * p->mass;
		cVector3d fg = cVector3d(9.81,0,0) * p->mass;
		cVector3d fs = cVector3d(0,0,0);
		cVector3d fd = -p->damping * p->velocity;
		
		for (spring *s : p->springs) {
			double dist = (s->p1->position - s->p2->position).length();
			double scalar = s->k*(dist - s->restlength);;
			if (p == s->p2) scalar *= -1;
			
			cVector3d dir;
			(s->p2->position - s->p1->position).normalizer(dir);
			double dscalar = -s->dk * (cDot(s->p1->velocity, dir) + cDot(s->p2->velocity, dir));
			
			fs += ((scalar + dscalar) * dir);
			
			
			s->line->m_pointA = s->p1->position;
			s->line->m_pointB = s->p2->position;
		
		}
		cVector3d fnet = fg + fs + fd + p->sphereForce;
//		cVector3d fnet = fs + fd + p->sphereForce;
		cVector3d acceleration = fnet/p->mass;
		p->velocity += (acceleration*dt);
		p->position += (p->velocity*dt);
		
		p->msphere->setLocalPos(p->position);
		
		cVector3d pdist = (p->position-o->particles[0]->position);
//		cVector3d pdist = p->position - pos;
//		cVector3d pdist = contactForce;
//		cVector3d pdist = contactForce/500;
//		if (pdist.length() < (p->radius+0.0005f)) { //cursor radius
//		cout << wireID << ":" <<p->sphereForce.length() << endl;
//		cout << wireID << ":" <<contactForce.length() << endl;
		if ((wireID == i && contactForce.length()>19) ||p->sphereForce.length()>0) {
			cVector3d fp = cVector3d(1,0,0)*std::min(contactForce.length(),20.0);
			p->sphereForce = -fp;
//			fdevice += fp;
			pressed = true;
		}
		else {
			p->sphereForce = cVector3d(0,0,0);
			pressed = false;
		}
	}	
//	return fdevice;
	return pressed;
}

void makeButton(pbutton *o) {
	particle *p0 = new particle();
	makeParticle(p0, 0.001, 0.1, 1.0, cVector3d(-0.014, -0.00, 0.005), true);
	p0->sphere->m_material->setRed();
	o->particles.push_back(p0);
	
	particle *p = new particle();
	makeParticle(p, 0.005, .5, 10.0, cVector3d(-0.004, -0.00, 0.005), false);
	p->sphere->m_material->setRed();
	o->particles.push_back(p);
	
	spring *s = new spring();
	makeSpring(s, 4000, 100, 0.01, p0, p);
	p0->springs.push_back(s);
	p->springs.push_back(s);
//	o->particles[0]->springs.push_back(s);
//	o->particles[1]->springs.push_back(s);
	
//	o->particles.push_back(p);
	o->springs.push_back(s);
	
}


cWorld* world;
cCamera* camera;
cSpotLight* light;
cSpotLight* backlight;
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
cVector3d toolInitPos(0.0,0.0,0.01);

cMultiMesh* bomb;
cMesh * deathScreen;
cMesh * winScreen;
cMesh * background;
cMesh * table;
cMesh * scratchSurface;
vector<cMultiMesh*> panels;
bool gameOver = false;

pbutton *bigbutton;
pbutton *lockbutton;

///////////////////////////////////////////////////

int timeLimit[4] = {0,5,0,0}; // mm:ss
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

int brailleOrder[4] = {3, 1, 2, 0}; // will be modified later on
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
bool wiresMade = false;
vector<cMultiMesh*> wires;
const string wireModels[4] = 
{
    "wire1.obj",
    "wire2.obj",
    "wire3.obj",
    "wire4.obj",
};

int wireSequence = 0;
vector<cMultiMesh*> cutWires;
const string cutWireModels[4] = 
{
    "cutwire1.obj",
    "cutwire2.obj",
    "cutwire3.obj",
    "cutwire4.obj",
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

vector<cMesh*> lockDials;

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
    wireColors[0].setGreen();
    wireColors[1].setRed();
    wireColors[2].setWhite();
    wireColors[3].setYellow();
}

void RemoveWorld()
{
    world->removeChild(bomb);
    world->removeChild(table);
    world->removeChild(background);
}

void GameOver(bool win)
{
    cout << win << "\n";
    RemoveWorld();
    gameOver = true;
    if (win)
        world->addChild(winScreen);
    else
        world->addChild(deathScreen);
}

void CreateEndGameScreens()
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

    winScreen = new cMesh();
    cCreatePlane(winScreen, 0.11, 0.06, cVector3d(0, -0.001, 0.03));
    winScreen->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
    winScreen->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));

    albedoMap = cTexture2d::create();
    albedoMap->loadFromFile("textures/winscreen.jpg");
    albedoMap->setWrapModeS(GL_REPEAT);
    albedoMap->setWrapModeT(GL_REPEAT);
    albedoMap->setUseMipmaps(true);
    winScreen->m_texture = albedoMap;
    winScreen->setUseTexture(true);
}

void CreateEnvironment()
{
    background = new cMesh();
    cCreatePlane(background, 0.22, 0.165, cVector3d(0, 0, -0.045));
    background->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
    background->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));

    cTexture2dPtr albedoMap = cTexture2d::create();
    albedoMap->loadFromFile("textures/background.png");
    albedoMap->setWrapModeS(GL_REPEAT);
    albedoMap->setWrapModeT(GL_REPEAT);
    albedoMap->setUseMipmaps(true);
    background->m_texture = albedoMap;
    background->setUseTexture(true);

    world->addChild(background);

   //////////////////////////////////

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
		material->id = i;
        wire->setLocalPos(cVector3d(posZ, posX + (i*spacing), posY - (i*offsetY)));
        wire->rotateAboutGlobalAxisRad(cVector3d(0,0,-1), cDegToRad(90));
		wires.push_back(wire);
        panels[0]->addChild(wire);
    }
    wiresMade = true;
}

void CreateCutWires()
{
    double posX = -0.0035;
    double posY = 0.0005;
    double posZ = 0.0025;
    double spacing = 0.003;
    double offsetY = 0.0001;
    for (int i = 0; i < 4; i++)
    {
        cMultiMesh* wire = new cMultiMesh();
        wire->loadFromFile("models/" + cutWireModels[i]);
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
		cutWires.push_back(wire);
    }
}

void CreateWireCover()
{
    cMultiMesh* cover = new cMultiMesh();
    cover->loadFromFile("models/wirecover.obj");
    cover->createAABBCollisionDetector(toolRadius);
    cover->computeBTN();

    cMesh* mesh = cover->getMesh(0);
    mesh->m_material = MyMaterial::create();
    mesh->m_material->setWhite();
    mesh->m_material->setUseHapticShading(true);
    cover->setStiffness(2000.0, true);

    cTexture2dPtr albedoMap = cTexture2d::create();
    albedoMap->loadFromFile("textures/wirecover.png");
    albedoMap->setWrapModeS(GL_REPEAT);
    albedoMap->setWrapModeT(GL_REPEAT);
    albedoMap->setUseMipmaps(true);
    mesh->m_texture = albedoMap;
    mesh->setUseTexture(true);
    mesh->setUseTransparency(true);

    MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(mesh->m_material);
    material->hasTexture = false;

    cover->setLocalPos(cVector3d(0.0025, 0.0001, -0.0005));
    cover->rotateAboutGlobalAxisRad(cVector3d(0,0,1), cDegToRad(90));
    panels[0]->addChild(cover);
}

// right pos : 0.019
// left pos : - 0.019
// top = 0.0095
// bottom = -0.0095

// front = 0.006
// behind: 0.0125

void CreateButton(pbutton *o) {
	particle *p0 = new particle();
	makeParticle(p0, 0.001, 0.1, 1.0, cVector3d(-0.014, -0.00, 0.005), true);
	p0->sphere->m_material->setRed();
	o->particles.push_back(p0);
	
	particle *p = new particle();
	makeParticle(p, 0.005, .5, 10.0, cVector3d(-0.004, -0.00, 0.005), false);
	p->sphere->m_material->setRed();
	o->particles.push_back(p);
	
	spring *s = new spring();
	makeSpring(s, 8000, 100, 0.01, p0, p);
	p0->springs.push_back(s);
	p->springs.push_back(s);
	o->springs.push_back(s);
	
/*	for (particle *b : o->particles) {
		b->sphere->setLocalPos(b->position);
		bomb->addChild(b->sphere);
	}
	*/
	particle *b = o->particles[1];
/*	cMesh *mesh = new cMesh();
	cCreateSphere(mesh, b->radius, 10, 5, b->position);
	mesh->createBruteForceCollisionDetector();
    mesh->computeBTN();
    mesh->m_material = MyMaterial::create();
	mesh->m_material->setWhite();
	mesh->m_material->setUseHapticShading(true);
	mesh->setStiffness(2000.0, true);
	mesh->setFriction(5, 3.5);
	MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(mesh->m_material);
	material->hasTexture = false;
	bomb->addChild(mesh);
	b->sphere = mesh;
*/
	b->msphere = new cMesh();
//	cCreateSphere(b->msphere, b->radius, 10, 5, b->position);
	cCreateCylinder(b->msphere, 2*b->radius, b->radius, 10, 10, 10, true, true, b->position);
        b->msphere->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
//        b->msphere->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));
	b->msphere->createBruteForceCollisionDetector();
    b->msphere->computeBTN();
    b->msphere->m_material = MyMaterial::create();
	b->msphere->m_material->setRed();
	b->msphere->m_material->setUseHapticShading(true);
	b->msphere->setStiffness(2000.0, true);
//	b->msphere->setFriction(1.0, .1);
	MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(b->msphere->m_material);
	material->hasTexture = false;
	material->id = 7;
	bomb->addChild(b->msphere);
//	b->sphere = mesh;

}

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
//        material->id = -1;
//		if (i==1) material->id =7;
//		if (i==3) material->id =8;

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

void CreateBrailleOrder() 
{
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

/*	for (int i=0; i<tempOrder.size(); i++) {
		std::cout << tempOrder[i];// <<std::endl;
	}
	std::cout<<std::endl;
	*/
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

void CreateBrailleTextures() 
{
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
	cCreatePlane(mesh, 0.01, 0.01, cVector3d(0,0,0));//cVector3d(0.0125, 0.0065, 0.005));

    mesh->createBruteForceCollisionDetector();
    mesh->rotateAboutGlobalAxisDeg(cVector3d(0, 1, 0), 90);
    mesh->rotateAboutGlobalAxisRad(cVector3d(1, 0, 0), cDegToRad(90));
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
	
	panels[7]->addChild(mesh);
	
}

void CreateScratchAndWin()
{
    scratchSurface = new cMesh();
    cCreatePlane(scratchSurface, 0.01, 0.01, cVector3d(0,0,0));//cVector3d(0.0125, 0.0065, 0.005));

    scratchSurface->createBruteForceCollisionDetector();
    scratchSurface->rotateAboutGlobalAxisDeg(cVector3d(0, 1, 0), 90);
    scratchSurface->rotateAboutGlobalAxisRad(cVector3d(1, 0, 0), cDegToRad(90));
    scratchSurface->scale(1.5);
    scratchSurface->setUseTransparency(true, true);

    scratchSurface->m_material = MyMaterial::create();
    scratchSurface->m_material->setWhite();
    scratchSurface->m_material->setUseHapticShading(true);
    scratchSurface->setStiffness(2000.0, true);

	MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(scratchSurface->m_material);

    cTexture2dPtr albedoMap = cTexture2d::create();
    albedoMap->loadFromFile("textures/bamboo-wood-albedo.png");
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

    scratchSurface->m_texture = albedoMap;
    material->m_height_map = heightMap;
    material->m_roughness_map = roughnessMap;
    material->hasTexture = true;

	scratchSurface->setUseTexture(true);

    cNormalMapPtr normalMap = cNormalMap::create();
    normalMap->createMap(scratchSurface->m_texture);
    scratchSurface->m_normalMap = normalMap;

    // set haptic properties
    scratchSurface->m_material->setStaticFriction(0.30);
    scratchSurface->m_material->setDynamicFriction(0.20);
    scratchSurface->m_material->setHapticTriangleSides(true, false);
    scratchSurface->m_material->setTextureLevel(1.5);
	
	panels[2]->addChild(scratchSurface);
}

void CreateLockPad(pbutton *o)
{
		cMesh * mesh = new cMesh();
	cCreatePlane(mesh,0.01,0.01,cVector3d(0,0,0));
	    mesh->createBruteForceCollisionDetector();
        mesh->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
//        mesh->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));
	    mesh->setLocalPos(cVector3d(0.0068,-0.019,-0.008));
//	    mesh->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
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
    material->id = 8;

	mesh->setUseTexture(true);
	
	
	
	cVector3d pos(0,-0.0027,-0.0055);
	cVector3d gap(0,0,0.0035);
	for (int i=0; i<3; i++) {
		cMesh* m = new cMesh();
		cCreateCylinder(m, 0.0025, 0.005, 6, 1, 1, true, true, pos+i*gap);
        m->rotateAboutGlobalAxisDeg(cVector3d(0,-1,0), 90);

		m->createBruteForceCollisionDetector();
		m->computeBTN();
		m->m_material = MyMaterial::create();
		m->m_material->setRed();
		m->m_material->setUseHapticShading(true);
		m->setStiffness(2000.0, true);
		m->setFriction(2.0, .5);
		MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(m->m_material);
		material->hasTexture = false;
		material->id = 9+i;
		lockDials.push_back(m);
		mesh->addChild(m);
	}
	
	
	particle *p0 = new particle();
	makeParticle(p0, 0.001, 0.1, 1.0, cVector3d(-0.006, -0.007, -0.001), true);
	p0->sphere->m_material->setRed();
	o->particles.push_back(p0);
	
	particle *p = new particle();
	makeParticle(p, 0.0015, .5, 10.0, cVector3d(0.004, -0.007, -0.001), false);
	p->sphere->m_material->setRed();
	o->particles.push_back(p);
	
	spring *s = new spring();
	makeSpring(s, 7000, 100, 0.01, p0, p);
	p0->springs.push_back(s);
	p->springs.push_back(s);
	o->springs.push_back(s);
	
	particle *b = o->particles[1];
	b->msphere = new cMesh();
//	cCreateSphere(b->msphere, b->radius, 10, 5, b->position);
	cCreateCylinder(b->msphere, 4*b->radius, b->radius, 10, 10, 10, true, true, b->position);
        b->msphere->rotateAboutLocalAxisDeg(cVector3d(0,1,0), 90);
//        b->msphere->rotateAboutGlobalAxisRad(cVector3d(1,0,0), cDegToRad(90));
	b->msphere->createBruteForceCollisionDetector();
    b->msphere->computeBTN();
    b->msphere->m_material = MyMaterial::create();
	b->msphere->m_material->setRed();
	b->msphere->m_material->setUseHapticShading(true);
	b->msphere->setStiffness(2000.0, true);
	b->msphere->setFriction(2.5, 1.0);
	MyMaterialPtr material0 = std::dynamic_pointer_cast<MyMaterial>(b->msphere->m_material);
	material0->hasTexture = false;
	material0->id = 8;
	bomb->addChild(b->msphere);
//	mesh->rotateAboutGlobalAxisDeg(cVector3d(0,1,0), 90);
	
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
        GameOver(false);
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

void RotateObjectsWithDevice(double timeInterval) 
{
    cVector3d angVel(0.0, 0.0, 0.4);

//	double timeInterval = 0.001;
	const double INERTIA = 0.4;
        const double MAX_ANG_VEL = 10.0;
        const double DAMPING = 2.8;

        // get position of cursor in global coordinates
        cVector3d toolPos = tool->getDeviceGlobalPos();

//	for (cMesh *m : lockDials) {
	cMesh *m;
	if (wireID >8 && wireID <12)
		m = lockDials[wireID-9];
	else return;

        // get position of object in global coordinates
        cVector3d objectPos = m->getGlobalPos();

        // compute a vector from the center of mass of the object (point of rotation) to the tool
        cVector3d v = cSub(toolPos, objectPos);

        // compute angular acceleration based on the interaction forces
        // between the tool and the object
        cVector3d angAcc(0,0,0);
        if (v.length() > 0.0)
        {
            // get the last force applied to the cursor in global coordinates
            // we negate the result to obtain the opposite force that is applied on the
            // object
            cVector3d toolForce = -tool->getDeviceGlobalForce();
//            cVector3d toolForce = -contactForce;

            // compute the effective force that contributes to rotating the object.
            cVector3d force = toolForce - cProject(toolForce, v);

            // compute the resulting torque
            cVector3d torque = cMul(v.length(), cCross( cNormalize(v), force));

            // update rotational acceleration
            angAcc = (1.0 / INERTIA) * torque;
        }

        // update rotational velocity
        angVel.add(timeInterval * angAcc);

        // set a threshold on the rotational velocity term
        double vel = angVel.length();
        if (vel > MAX_ANG_VEL)
        {
            angVel.mul(MAX_ANG_VEL / vel);
        }

        // add some damping too
        angVel.mul(1.0 - DAMPING * timeInterval);

        // if user switch is pressed, set angular velocity to zero
        if (tool->getUserSwitch(0) == 1)
        {
            angVel.zero();
        }

        // compute the next rotation configuration of the object
        if (angVel.length() > C_SMALL)
        {
            m->rotateAboutGlobalAxisRad(cNormalize(angVel), timeInterval * angVel.length());
        }
//	}
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

   ///////////////////////////////////////////////////

    light = new cSpotLight(world);
    world->addChild(light);

    light->setEnabled(true);
    // light->setLocalPos(0.7, 0.3, 1.5);
    // light->setDir(-0.5, -0.2, -0.8);

    light->setLocalPos(0.7, 0.3, 1.0);
    light->setDir(-0.5, -0.2, -0.8);
    light->setShadowMapEnabled(true);
    light->m_shadowMap->setQualityHigh();
    light->setCutOffAngleDeg(10);

   ///////////////////////////////////////////////////

    // backlight = new cSpotLight(world);
    // world->addChild(backlight);

    // backlight->setEnabled(true);
    // backlight->setLocalPos(0.4, 0.3, 1.6);
    // backlight->setDir(-0.7, -0.2, -1.0);
    // backlight->setShadowMapEnabled(true);
    // backlight->m_shadowMap->setQualityHigh();
    // backlight->setCutOffAngleDeg(20);

    //--------------------------------------------------------------------------
    // HAPTIC DEVICE
    //--------------------------------------------------------------------------

    handler = new cHapticDeviceHandler();
    handler->getDevice(hapticDevice, 0);
    
/*    
    // open a connection to haptic device
    hapticDevice->open();

    // calibrate device (if necessary)
    hapticDevice->calibrate();

    // retrieve information about the current haptic device
    cHapticDeviceInfo info = hapticDevice->getSpecifications();

    // display a reference frame if haptic device supports orientations
    if (info.m_sensedRotation == true)
    {
        // display reference frame
        cursor->setShowFrame(true);

        // set the size of the reference frame
        cursor->setFrameSize(0.05);
    }
*/
    hapticDevice->setEnableGripperUserSwitch(true);

    tool = new cToolCursor(world);
    world->addChild(tool);

   //

    // [CPSC.86] replace the tool's proxy rendering algorithm with our own
    proxyAlgorithm = new MyProxyAlgorithm;
    delete tool->m_hapticPoint->m_algorithmFingerProxy;
    tool->m_hapticPoint->m_algorithmFingerProxy = proxyAlgorithm;
    tool->m_hapticPoint->m_sphereProxy->m_material->setWhite();
    
    tool->setRadius(0.0005, toolRadius);
//    tool->setLocalPos(toolInitPos);
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
    // MAIN GRAPHIC LOOP
    //--------------------------------------------------------------------------
    

    // Initial setup
    SetColors();
    CreateEnvironment();
    CreateBomb();
    CreatePanels();

    // Wires
    CreateWires();
    CreateCutWires();
    CreateWireCover();

    // Timer
    CreateNumberTextures();
    CreateTimer();
    UpdateTimer();

    // Braille puzzle
	CreateBrailleOrder();
	CreateBrailleTextures();
	CreateBraillePuzzle();  
    CreateBrailleLegend();
    

	bigbutton = new pbutton();
//	makeButton(bigbutton);
	CreateButton(bigbutton);
    lockbutton = new pbutton();
    CreateLockPad(lockbutton);

    // Scratch and Win
    CreateScratchAndWin();

    // End game
    CreateEndGameScreens();

    // After setup
    SetPanelPositions();
    
    
    
    //--------------------------------------------------------------------------
    // START SIMULATION
    //--------------------------------------------------------------------------

    hapticsThread = new cThread();
    hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

    atexit(close);
	///////////////////////////////////////////////////
    
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
   //////////////////////
    // UPDATE WIDGETS
   //////////////////////

    labelRates->setText(cStr(freqCounterGraphics.getFrequency(), 0) + " Hz / " +
        cStr(freqCounterHaptics.getFrequency(), 0) + " Hz");

    labelRates->setLocalPos((int)(0.5 * (width - labelRates->getWidth())), 15);


    labelDebug->setText(cStr(proxyAlgorithm->m_debugValue) +
                        ", " + cStr(proxyAlgorithm->m_debugVector.x()) +
                        ", " + cStr(proxyAlgorithm->m_debugVector.y()) +
                        ", " + cStr(proxyAlgorithm->m_debugVector.z())
                    );

    labelDebug->setLocalPos((int)(0.5 * (width - labelDebug->getWidth())), 40);


   //////////////////////
    // RENDER SCENE
   //////////////////////
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
    
    bool oldButton0 = false;
    bool oldButton1 = false;

    cPrecisionClock clock;
    clock.reset();


    while(simulationRunning)
    {
		
		
		        /////////////////////////////////////////////////////////////////////
        // SIMULATION TIME
        /////////////////////////////////////////////////////////////////////

        // stop the simulation clock
        clock.stop();

        // read the time increment in seconds
        double timeInterval = clock.getCurrentTimeSeconds();

        // restart the simulation clock
        clock.reset();
        clock.start();


       //////////////////////
        // READ HAPTIC DEVICE
       //////////////////////

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

        if (midPressed && !middle)
        {
			if (wireID >=0 && wireID <4) {// && !middle) {
//				std::cout << "cut wire: " << wireID << std::endl;
				cGenericObject * parent = wires[wireID]->getParent();
				parent->removeChild(wires[wireID]);
                parent->addChild(cutWires[wireID]);
                if (wireID == brailleOrder[wireSequence])
					wireSequence++;
				else
                    GameOver(false);

                if (wireSequence >= 4)
                    GameOver(true);
			}
			midPressed = false;
//			middle = false;
			
        }
        if (leftPressed)// && !left)
        {
            world->computeGlobalPositions();
            tool->updateFromDevice();
            tool->computeInteractionForces();
            tool->applyToDevice();
            
			bomb->rotateAboutLocalAxisDeg(cVector3d(0,0,-1), 0.25);
			leftPressed = false;
        }
        if (rightPressed)// && !right)
        {
            world->computeGlobalPositions();
            tool->updateFromDevice();
            tool->computeInteractionForces();
            tool->applyToDevice();

			bomb->rotateAboutLocalAxisDeg(cVector3d(0,0,1), 0.25);
			rightPressed = false;
        }
//        tool->setLocalRot(bomb->getLocalRot());
//        cVector3d force(0, 0, 0);

//		force = update(bigbutton, 0.001, position);
//		if (update(bigbutton, 0.001, position))
//			cout << "pressed" << endl;
//		else
//			cout << "released" << endl;
		bool curButton0 = checkButton(bigbutton, 0.001, 7);
		if (curButton0 != oldButton0) {
			if (!oldButton0 && curButton0)
				cout << "pressed\n";
			else
				cout << "released\n";
		}
		oldButton0 = curButton0;
		bool curButton1 = checkButton(lockbutton, 0.001, 8);
		if (curButton1 != oldButton1) {
			if (!oldButton1 && curButton1)
				cout << "pressed\n";
			else
				cout << "released\n";
		}
		oldButton1 = curButton1;

//        tool->setLocalPos(tool->getLocalPos());

//		std::cout << wireID << std::endl;
		world->computeGlobalPositions();

       //////////////////////
        // UPDATE 3D CURSOR MODEL
       //////////////////////
        
        tool->updateFromDevice();

       //////////////////////
        // COMPUTE FORCES
       //////////////////////
        tool->computeInteractionForces();


/*
	if (wireID < 0){
//		tool->setLocalPos(position);
//      tool->setLocalRot(rotation);

//		tool->setLocalPos(position);
//		cVector3d r0;
//      tool->setLocalRot(bomb->getLocalRot());


        cVector3d force(0, 0, 0);
        cVector3d torque(0, 0, 0);
        double gripperForce = 0.0;

//		cout << "dfd" << endl;
		force = update(bigbutton, 0.001, position);


        // send computed force, torque, and gripper force to haptic device
        hapticDevice->setForceAndTorqueAndGripperForce(force, torque, gripperForce);
	}
	else {
	*/	
		
        tool->applyToDevice();
//   tool->computeInteractionForces();
//             tool->applyToDevice();
//}
       //////////////////////
       
       RotateObjectsWithDevice(timeInterval);

        freqCounterHaptics.signal(1);
    }

    simulationFinished = true;
}

//------------------------------------------------------------------------------
