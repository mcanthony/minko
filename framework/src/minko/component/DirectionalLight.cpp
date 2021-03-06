/*
Copyright (c) 2014 Aerys

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

#include "minko/component/DirectionalLight.hpp"

#include "minko/math/Vector3.hpp"

using namespace minko;
using namespace minko::component;
using namespace minko::math;

DirectionalLight::DirectionalLight(float diffuse,
                                   float specular) :
    AbstractDiscreteLight("directionalLights", diffuse, specular),
    _worldDirection(Vector3::create(0.f, 0.f, -1.f))
{
    data()->set("direction", _worldDirection);
}

DirectionalLight::DirectionalLight(const DirectionalLight& directionalLight, const CloneOption& option) :
AbstractDiscreteLight("directionalLights", directionalLight.diffuse(), directionalLight.specular()),
	_worldDirection(Vector3::create(directionalLight.data()->get<Vector3::Ptr>("direction")))
{
	data()->set("direction", _worldDirection);
}

AbstractComponent::Ptr
DirectionalLight::clone(const CloneOption& option)
{
	auto light = std::shared_ptr<DirectionalLight>(new DirectionalLight(*this, option));

	light->initialize();

	return light;
}

void
DirectionalLight::updateModelToWorldMatrix(std::shared_ptr<Matrix4x4> modelToWorld)
{
    modelToWorld->deltaTransform(Vector3::forward(), _worldDirection);
}
