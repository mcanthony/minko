/*
Copyright (c) 2013 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "minko/component/OculusVRCamera.hpp"

#include <OVR.h>

#include "minko/scene/Node.hpp"
#include "minko/scene/NodeSet.hpp"
#include "minko/component/SceneManager.hpp"
#include "minko/component/Renderer.hpp"
#include "minko/component/PerspectiveCamera.hpp"
#include "minko/component/Transform.hpp"
#include "minko/component/Surface.hpp"
#include "minko/geometry/QuadGeometry.hpp"
#include "minko/data/StructureProvider.hpp"
#include "minko/render/Texture.hpp"
#include "minko/file/AssetLibrary.hpp"
#include "minko/math/Matrix4x4.hpp"
#include "minko/render/Effect.hpp"

using namespace minko;
using namespace minko::scene;
using namespace minko::component;
using namespace minko::math;

/*static*/ const float					OculusVRCamera::WORLD_UNIT	= 1.0f;
/*static*/ const unsigned int			OculusVRCamera::TARGET_SIZE	= 2048;

OculusVRCamera::OculusVRCamera(float aspectRatio, float zNear, float zFar) :
	_aspectRatio(aspectRatio),
	_zNear(zNear),
	_zFar(zFar),
	_ovrSystem(new OVR::System()),
	_sceneManager(nullptr),
	_root(nullptr),
	_leftCamera(nullptr),
	_rightCamera(nullptr),
	_renderer(nullptr),
	_targetAddedSlot(nullptr),
	_targetRemovedSlot(nullptr),
	_addedSlot(nullptr),
	_removedSlot(nullptr),
	_renderEndSlot(nullptr)
{

}

OculusVRCamera::~OculusVRCamera()
{
	delete _ovrSystem;
}


void
OculusVRCamera::initialize()
{
	_targetAddedSlot = targetAdded()->connect(std::bind(
		&OculusVRCamera::targetAddedHandler, shared_from_this(), std::placeholders::_1, std::placeholders::_2
	));

	_targetRemovedSlot = targetRemoved()->connect(std::bind(
		&OculusVRCamera::targetRemovedHandler, shared_from_this(), std::placeholders::_1, std::placeholders::_2
	));

	const bool ovrInitOK = initializeOVRDevice();
	if (ovrInitOK)
		std::cerr << "Failed to dynamically retrieve HDM parameters" << std::endl;
}

void
OculusVRCamera::resetOVRDevice()
{
	_hmdInfo.hResolution			= 1280.0f;
	_hmdInfo.vResolution			= 800.0f;
	_hmdInfo.hScreenSize			= 0.14976f;
	_hmdInfo.vScreenSize			= _hmdInfo.hScreenSize / (1280.0f / 800.0f);
	_hmdInfo.vScreenCenter			= 0.5f * _hmdInfo.vScreenSize;
	_hmdInfo.interpupillaryDistance	= 0.064f;
	_hmdInfo.lensSeparationDistance	= 0.0635f;
	_hmdInfo.eyeToScreenDistance	= 0.041f;
	_hmdInfo.distortionK			= Vector4::create(1.0f, 0.22f, 0.24f, 0.0f);
}

bool
OculusVRCamera::initializeOVRDevice()
{
	resetOVRDevice();

	OVR::Ptr<OVR::DeviceManager>	deviceManager	= *OVR::DeviceManager::Create();
	if (deviceManager == nullptr)
		return false;

	OVR::HMDInfo					hmdInfo;
	OVR::Ptr<OVR::HMDDevice>		hmdDevice		= *deviceManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
	if (hmdDevice == nullptr)
		return false;

	hmdDevice->GetDeviceInfo(&hmdInfo);
	_hmdInfo.hResolution			= (float)hmdInfo.HResolution;
	_hmdInfo.vResolution			= (float)hmdInfo.VResolution;
	_hmdInfo.hScreenSize			= hmdInfo.HScreenSize;
	_hmdInfo.vScreenSize			= hmdInfo.VScreenSize;
	_hmdInfo.vScreenCenter			= hmdInfo.VScreenCenter;
	_hmdInfo.interpupillaryDistance	= hmdInfo.InterpupillaryDistance;
	_hmdInfo.lensSeparationDistance	= hmdInfo.LensSeparationDistance;
	_hmdInfo.eyeToScreenDistance	= hmdInfo.EyeToScreenDistance;
	_hmdInfo.distortionK			= Vector4::create(hmdInfo.DistortionK[0], hmdInfo.DistortionK[1], hmdInfo.DistortionK[2], hmdInfo.DistortionK[3]);

	return true;
}

void
OculusVRCamera::targetAddedHandler(AbsCmpPtr component, NodePtr target)
{
	if (targets().size() > 1)
		throw std::logic_error("The OculusVRCamera component cannot have more than 1 target.");

	_addedSlot = target->added()->connect(std::bind(
		&OculusVRCamera::addedHandler,
		shared_from_this(),
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3
	));

	_removedSlot = target->removed()->connect(std::bind(
		&OculusVRCamera::removedHandler,
		shared_from_this(),
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3
	));

	addedHandler(target->root(), target, target->parent());
}


void
OculusVRCamera::targetRemovedHandler(AbsCmpPtr component, NodePtr target)
{
	_addedSlot		= nullptr;
	_removedSlot	= nullptr;

	removedHandler(target->root(), target, target->parent());
}

void
OculusVRCamera::addedHandler(NodePtr node, NodePtr target, NodePtr parent)
{
	findSceneManager();
}

void
OculusVRCamera::removedHandler(NodePtr node, NodePtr target, NodePtr parent)
{
	findSceneManager();
}

void
OculusVRCamera::findSceneManager()
{
	NodeSet::Ptr roots = NodeSet::create(targets())
		->roots()
		->where([](NodePtr node)
		{
			return node->hasComponent<SceneManager>();
		});

	if (roots->nodes().size() > 1)
		throw std::logic_error("OculusVRCamera cannot be in two separate scenes.");
	else if (roots->nodes().size() == 1)
		setSceneManager(roots->nodes()[0]->component<SceneManager>());		
	else
		setSceneManager(nullptr);
}

void
OculusVRCamera::setSceneManager(SceneManager::Ptr sceneManager)
{
	if (_sceneManager == sceneManager)
		return;

	if (sceneManager == nullptr)
	{
		_sceneManager	= nullptr;
		_renderer		= nullptr;
		_leftCamera		= nullptr;
		_rightCamera	= nullptr;

		if (_root)
			targets().front()->removeChild(_root);

		_root			= nullptr;
		_renderEndSlot	= nullptr;

		return;
	}

	_sceneManager	= sceneManager;
	auto context	= _sceneManager->assets()->context();

	getHMDInfo(_hmdInfo);

	// ffxa
	auto	pixelOffset					= Vector2::create(1.0f / TARGET_SIZE, 1.0f / TARGET_SIZE);

	// distortion scale
	const auto distortionLensShift		= 1.0f - 2.0f * _hmdInfo.lensSeparationDistance / _hmdInfo.hScreenSize;
	const auto fitRadius				= abs(- 1.0f - distortionLensShift);
	const auto distortionScale			= distort(fitRadius) / fitRadius;

	// projection matrix 
	const auto screenAspectRatio		= (_hmdInfo.hResolution * 0.5f) / _hmdInfo.vResolution;
	const auto screenFOV				= 2.0f * atanf(distortionScale * (_hmdInfo.vScreenSize * 0.5f) / _hmdInfo.eyeToScreenDistance);

	const auto viewCenter				= _hmdInfo.hScreenSize * 0.25f;
	const auto eyeProjectionShift		= viewCenter - _hmdInfo.lensSeparationDistance * 0.5f;
	const auto projectionCenterOffset	= 4.0f * eyeProjectionShift / _hmdInfo.hScreenSize; // in clip coordinates

	// view transform translation in world units
	const auto halfIPD					= 0.5f * _hmdInfo.interpupillaryDistance * WORLD_UNIT;

	// left eye
	auto leftEye						= scene::Node::create();
	auto leftEyeTexture					= render::Texture::create(context, TARGET_SIZE, TARGET_SIZE, false, true);
	auto leftRenderer					= Renderer::create();
	auto leftPostProjection				= Matrix4x4::create()->appendTranslation(+ projectionCenterOffset, 0.0f, 0.0f);

	_leftCamera = PerspectiveCamera::create(screenAspectRatio, screenFOV, _zNear, _zFar, leftPostProjection);

	leftEyeTexture->upload();
	leftRenderer->target(leftEyeTexture);
	leftEye->addComponent(leftRenderer);
	leftEye->addComponent(_leftCamera);
	leftEye->addComponent(Transform::create(
		Matrix4x4::create()->appendTranslation(- halfIPD, 0.0f, 0.0f) // view transform
	));

	// right eye
	auto rightEye						= scene::Node::create();
	auto rightEyeTexture				= render::Texture::create(context, TARGET_SIZE, TARGET_SIZE, false, true);
	auto rightRenderer					= Renderer::create();
	auto rightPostProjection			= Matrix4x4::create()->appendTranslation(- projectionCenterOffset, 0.0f, 0.0f);

	_rightCamera = PerspectiveCamera::create(screenAspectRatio, screenFOV, _zNear, _zFar, rightPostProjection);

	rightEyeTexture->upload();
	rightRenderer->target(rightEyeTexture);
	rightEye->addComponent(rightRenderer);
	rightEye->addComponent(_rightCamera);
	rightEye->addComponent(Transform::create(
		Matrix4x4::create()->appendTranslation(+ halfIPD, 0.0f, 0.0f) // view transform
	));

	_root = scene::Node::create("oculusvr");
	_root->addChild(leftEye)->addChild(rightEye);
	targets().front()->addChild(_root);

	// post processing effect
	const auto clipWidth		= 0.5f;
	const auto clipHeight		= 1.0f;

	auto leftScreenCorner		= Vector2::create(0.0f, 0.0f);
	auto leftScreenCenter		= leftScreenCorner + Vector2::create(clipWidth * 0.5f, clipHeight * 0.5f);
	auto leftLensCenter			= leftScreenCorner + Vector2::create(
		(clipWidth + distortionLensShift * 0.5f) * 0.5f,
		clipHeight * 0.5f
	);

	auto rightScreenCorner		= Vector2::create(0.5f, 0.0f);
	auto rightScreenCenter		= rightScreenCorner + Vector2::create(clipWidth * 0.5f, clipHeight * 0.5f);
	auto rightLensCenter		= rightScreenCorner + Vector2::create(
		(clipWidth - distortionLensShift * 0.5f) * 0.5f,
		clipHeight * 0.5f
	);

	auto scalePriorDistortion	= Vector2::create(2.0f / clipWidth, (2.0f / clipHeight) / screenAspectRatio);
	auto scaleAfterDistortion	= Vector2::create((clipWidth * 0.5f) / distortionScale, ((clipHeight * 0.5f) * screenAspectRatio / distortionScale));

#ifdef DEBUG_OCULUS
	std::cout << "- distortion lens shift\t= " << distortionLensShift << std::endl;
	std::cout << "- distortion scale\t= " << distortionScale << std::endl;
	std::cout << "- radial distortion\t= " << _hmdInfo.distortionK->toString() << std::endl;
	std::cout << "- left lens center\t= " << leftLensCenter->toString() << "\n- left screen center\t= " << leftScreenCenter->toString() << "\n- left screen corner\t= " << leftScreenCorner->toString() << std::endl;
	std::cout << "- right lens center\t= " << rightLensCenter->toString() << "\n- right screen center\t= " << rightScreenCenter->toString() << "\n- right screen corner\t= " << rightScreenCorner->toString() << std::endl;
	std::cout << "- scale prior distortion\t= " << scalePriorDistortion->toString() << "\n- scale after distortion\t= " << scaleAfterDistortion->toString() << std::endl;
#endif // DEBUG_OCULUS

	_renderer = Renderer::create();

	auto ppFx = _sceneManager->assets()->effect("effect/OculusVR/OculusVR.effect");

	if (!ppFx)
		throw std::logic_error("OculusVR.effect has not been loaded.");

	auto ppScene = scene::Node::create()
		->addComponent(_renderer)
		->addComponent(Surface::create(
			geometry::QuadGeometry::create(_sceneManager->assets()->context()),
			data::StructureProvider::create("oculusvr")
				->set("distortionK",			_hmdInfo.distortionK)
				->set("pixelOffset",			pixelOffset)
				->set("scalePriorDistortion",	scalePriorDistortion)
				->set("scaleAfterDistortion",	scaleAfterDistortion)
				->set("leftEyeTexture",			leftEyeTexture)
				->set("leftLensCenter",			leftLensCenter)
				->set("leftScreenCorner",		leftScreenCorner)
				->set("leftScreenCenter",		leftScreenCenter)
				->set("rightEyeTexture",		rightEyeTexture)
				->set("rightLensCenter",		rightLensCenter)
				->set("rightScreenCorner",		rightScreenCorner)
				->set("rightScreenCenter",		rightScreenCenter),
			ppFx
		));

	_renderEndSlot = sceneManager->renderingEnd()->connect(std::bind(
		&OculusVRCamera::renderEndHandler,
		shared_from_this(),
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3
	));
}

void
OculusVRCamera::renderEndHandler(std::shared_ptr<SceneManager>		sceneManager,
								 uint								frameId,
								 std::shared_ptr<render::Texture>	renderTarget)
{
	_renderer->render(sceneManager->assets()->context());
}

bool
OculusVRCamera::getHMDInfo(HMD& hmd) const
{
	// should retrieve the device's physical parameters dynamically
	// fake values for the time being
	hmd.hResolution				= 1280.0f;
	hmd.vResolution				= 800.0f;
	hmd.hScreenSize				= 0.14976f;
	hmd.vScreenSize				= hmd.hScreenSize / (1280.0f / 800.0f);
	hmd.vScreenCenter			= 0.5f * hmd.vScreenSize;
	hmd.interpupillaryDistance	= 0.064f;
	hmd.lensSeparationDistance	= 0.0635f;
	hmd.eyeToScreenDistance		= 0.041f;
	hmd.distortionK				= Vector4::create(1.0f, 0.22f, 0.24f, 0.0f);




	return false;
}

float
OculusVRCamera::distort(float r) const
{
	const float r2 = r * r;
	const float r4 = r2 * r2;
	const float r6 = r4 * r2;

	return r * (
		_hmdInfo.distortionK->x()
		+ r2 * _hmdInfo.distortionK->y()
		+ r4 * _hmdInfo.distortionK->z()
		+ r6 * _hmdInfo.distortionK->w()
	);
}