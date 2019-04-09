//==============================================================================
/*
    CPSC 599.86 / 601.86 - Computer Haptics
    Winter 2018, University of Calgary

    This class extends the cAlgorithmFingerProxy class in CHAI3D that
    implements the god-object/finger-proxy haptic rendering algorithm.
    It allows us to modify or recompute the force that is ultimately sent
    to the haptic device.

    Your job for this assignment is to implement the updateForce() method
    in this class to support for two new effects: force shading and haptic
    textures. Methods for both are described in Ho et al. 1999.
*/
//==============================================================================

#include "MyProxyAlgorithm.h"
#include "MyMaterial.h"

using namespace chai3d;

//==============================================================================
/*!
    This method uses the information computed earlier in
    computeNextBestProxyPosition() to calculate the force to be rendered.
    It first calls cAlgorithmFingerProxy::updateForce() to compute the base
    force from contact geometry and the constrained proxy position. That
    force can then be modified or recomputed in this function.

    Your implementation of haptic texture mapping will likely end up in this
    function. When this function is called, collision detection has already
    been performed, and the proxy point has already been updated based on the
    constraints found. Your job is to compute a force with all that information
    available to you.

    Useful variables to read:
        m_deviceGlobalPos   - current position of haptic device
        m_proxyGlboalPos    - computed position of the constrained proxy
        m_numCollisionEvents- the number of surfaces constraining the proxy
        m_collisionRecorderConstraint0,1,2
                            - up to three cCollisionRecorder structures with
                              cCollisionEvents that contain very useful
                              information about each contact

    Variables that this function should set/reset:
        m_normalForce       - computed force applied in the normal direction
        m_tangentialForce   - computed force along the tangent of the surface
        m_lastGlobalForce   - this is what the operator ultimately feels!!!
*/
//==============================================================================
extern int wireID;
extern cVector3d contactForce;
extern cVector3d returnForce;

void MyProxyAlgorithm::updateForce()
{
    // get the base class to do basic force computation first
    cAlgorithmFingerProxy::updateForce();

    // m_debugValue =  m_numCollisionEvents;
    // TODO: compute force shading and texture forces here

    if (m_numCollisionEvents > 0)
    {
        cCollisionEvent* c0 = &m_collisionRecorderConstraint0.m_nearestCollision;

        // Indices for three vertices of the triangle I am touching
        int vi0 = c0->m_triangles->getVertexIndex0(c0->m_index);
        int vi1 = c0->m_triangles->getVertexIndex1(c0->m_index);
        int vi2 = c0->m_triangles->getVertexIndex2(c0->m_index);

        // Normals for the three vertices I am touching
        cVector3d normal0 = c0->m_triangles->m_vertices->getNormal(vi0);
        cVector3d normal1 = c0->m_triangles->m_vertices->getNormal(vi1);
        cVector3d normal2 = c0->m_triangles->m_vertices->getNormal(vi2);

        m_debugValue = c0->m_index;
        m_debugVector = normal0;

		cVector3d texCoord = c0->m_triangles->getTexCoordAtPosition(c0->m_index, c0->m_localPos);

		// if (texCoord.x() < 0.0)
		// 	texCoord = texCoord + cVector3d(1.0, 0.0, 0.0);
		// else if (texCoord.x() > 1.0)
		// 	texCoord = texCoord - cVector3d(1.0, 0.0, 0.0);

		// if (texCoord.y() < 0.0)
		// 	texCoord = texCoord + cVector3d(0.0, 1.0, 0.0);
		// else if (texCoord.y() > 1.0)
		// 	texCoord = texCoord - cVector3d(0.0, 1.0, 0.0);

        // cVector3d surfaceNormal = c0->m_localNormal;
        cVector3d surfaceNormal = computeShadedSurfaceNormal(c0);
        MyMaterialPtr material = std::dynamic_pointer_cast<MyMaterial>(c0->m_object->m_material);
//		wireID = material->id;
        if (material)
        {
			wireID = material->id;
			cVector3d F = -material->getStiffness() * (m_deviceGlobalPos - m_proxyGlobalPos);
            double roughnessFactor = 5.0;
//				contactForce = cVector3d(0,0,0);
//            if (material->id == 7 ||material->id == 8 || contactForce.length()>0)
            if (material->id > 6)//  || contactForce.length()>0)
//				contactForce = (m_deviceGlobalPos - m_proxyGlobalPos);
				contactForce = F;
//			else
//				contactForce = cVector3d(0,0,0);
            if (!material->hasTexture) {
//				m_lastGlobalForce = -returnForce;
//				m_lastGlobalForce = cVector3d(0,0,0);
//				wireID = -1;
//contactForce = cVector3d(0,0,0);
				return;
			}
         
			
//			std::cout<< material->id << std::endl;

            
            // cImagePtr normalMap = material->m_normal_map->m_image;
            cImagePtr heightMap = material->m_height_map->m_image;
            cImagePtr roughnessMap = material->m_roughness_map->m_image;

            // cColorf normalPixelColor;
            cColorf heightPixelColor;
            cColorf roughnessPixelColor;

            // normalMap->getPixelColorInterpolated(x, y, normalPixelColor);
            
            double x = texCoord.x() * heightMap->getWidth();
            double y = texCoord.y() * heightMap->getHeight();
            heightMap->getPixelColor(x, y, heightPixelColor);

            x = texCoord.x() * roughnessMap->getWidth();
            y = texCoord.y() * roughnessMap->getHeight();
            roughnessMap->getPixelColor(x, y, roughnessPixelColor);

            // double normal = normalPixelColor.getR();// - 0.5;
            double height = heightPixelColor.getR();// - 0.5;
            double roughness = roughnessPixelColor.getR();// - 0.5;

            // std::cout << height << " <- height\n" << roughness << " <- roughness\n\n";

            cVector3d perturbedNormal = surfaceNormal * (height * 2.0 + 1);// * cNormalize(m_proxyGlobalPos);
            // perturbedNormal.normalize();

            cVector3d force = F.length() * perturbedNormal;

            cVector3d toolForce = cVector3d(0.0, 0.0, 0.0);
            double toolRoughness = 0;

            // Add the force calculated for the ray (force) and the tool (toolForce) together and apply it to the device
            m_lastGlobalForce = toolForce + force;

            // Add the tray roughness (roughness) and tool roughness (toolRoughness) together
            roughness += toolRoughness;

            roughness *= roughnessFactor;

            double mu_k = 0.8 * roughness;
            double mu_s = 1.0 * roughness;

            c0->m_object->setFriction(mu_s, mu_k);
        }
    }
//    else
		wireID = -1;
	contactForce = cVector3d(0,0,0);
}


//==============================================================================
/*!
    This method attempts to move the proxy, subject to friction constraints.
    This is called from computeNextBestProxyPosition() when the proxy is
    ready to move along a known surface.

    Your implementation of friction mapping will likely need to modify or
    replace the CHAI3D implementation in cAlgorithmFingerProxy. You may
    either copy the implementation from the base class and modify it to take
    into account a friction map, or use your own friction rendering from your
    previous assignment.

    The most important thing to do in this method is to write the desired
    proxy position into the m_nextBestProxyGlobalPos member variable.

    The input parameters to this function are as follows, all provided in the
    world (global) coordinate frame:

    \param  a_goal    The location to which we'd like to move the proxy.
    \param  a_proxy   The current position of the proxy.
    \param  a_normal  The surface normal at the obstructing surface.
    \param  a_parent  The surface along which we're moving.
*/
//==============================================================================
void MyProxyAlgorithm::testFrictionAndMoveProxy(const cVector3d& a_goal,
                                                const cVector3d& a_proxy,
                                                cVector3d &a_normal,
                                                cGenericObject* a_parent)
{
    cAlgorithmFingerProxy::testFrictionAndMoveProxy(a_goal, a_proxy, a_normal, a_parent);
}
