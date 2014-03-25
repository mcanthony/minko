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

#if defined(EMSCRIPTEN)

#include "minko/Common.hpp"
#include "emscripten/dom/EmscriptenDOMElement.hpp"
#include "emscripten/dom/EmscriptenDOMEvent.hpp"
#include "emscripten/dom/EmscriptenDOM.hpp"
#include "minko/dom/AbstractDOMEvent.hpp"
#include "emscripten/emscripten.h"

using namespace minko;
using namespace minko::dom;
using namespace emscripten;
using namespace emscripten::dom;


int
EmscriptenDOMElement::_elementUid = 0;

std::list<EmscriptenDOMElement::Ptr>
EmscriptenDOMElement::domElements;

std::map<std::string,EmscriptenDOMElement::Ptr>
EmscriptenDOMElement::_accessorToElement;

EmscriptenDOMElement::EmscriptenDOMElement(std::string jsAccessor) :
	_jsAccessor(jsAccessor),
	_onclick(Signal<AbstractDOMEvent::Ptr>::create()),
	_onmousedown(Signal<AbstractDOMEvent::Ptr>::create()),
	_onmousemove(Signal<AbstractDOMEvent::Ptr>::create()),
	_onmouseup(Signal<AbstractDOMEvent::Ptr>::create()),
	_onmouseover(Signal<AbstractDOMEvent::Ptr>::create()),
	_onmouseout(Signal<AbstractDOMEvent::Ptr>::create()),
	_onclickSet(false),
	_onmousedownSet(false),
	_onmousemoveSet(false),
	_onmouseupSet(false),
	_onmouseoverSet(false),
	_onmouseoutSet(false)
{
	std::string eval = jsAccessor + ".minkoName = '" + jsAccessor + "';";
	emscripten_run_script(eval.c_str());
}


EmscriptenDOMElement::Ptr
EmscriptenDOMElement::getDOMElement(std::string jsElement)
{
	std::string eval = "if (" + jsElement + ".minkoName) (" + jsElement + ".minkoName); else ('');";
	std::string minkoName = std::string(emscripten_run_script_string(eval.c_str()));

	if (minkoName == "")
	{
		minkoName = "Minko.element" + std::to_string(_elementUid++);

		eval = minkoName + " = " + jsElement;
		emscripten_run_script(eval.c_str());
	}
	else
	{
		auto i = _accessorToElement.find(minkoName);

		if (i != _accessorToElement.end())
			return i->second;
	}

	return create(minkoName);
}

EmscriptenDOMElement::Ptr
EmscriptenDOMElement::create(std::string jsAccessor)
{
	EmscriptenDOMElement::Ptr element(new EmscriptenDOMElement(jsAccessor));
	domElements.push_back(element);
	_accessorToElement[jsAccessor] = element;
	return element;
}

std::string
EmscriptenDOMElement::getJavascriptAccessor()
{
	return _jsAccessor;
}

std::string
EmscriptenDOMElement::id()
{
	std::string eval = "(" + _jsAccessor + ".id)";
	char* result = emscripten_run_script_string(eval.c_str());
	return std::string(result);
}

void
EmscriptenDOMElement::id(std::string newId)
{
	std::string eval = _jsAccessor + ".id = '" + newId + "';";
	emscripten_run_script(eval.c_str());
}

std::string
EmscriptenDOMElement::className()
{
	std::string eval = "(" + _jsAccessor + ".className)";
	char* result = emscripten_run_script_string(eval.c_str());
	return std::string(result);
}

void
EmscriptenDOMElement::className(std::string newClassName)
{
	std::string eval = _jsAccessor + ".className = '" + newClassName + "';";
	emscripten_run_script(eval.c_str());
}

std::string
EmscriptenDOMElement::tagName()
{
	std::string eval = "(" + _jsAccessor + ".tagName)";
	char* result = emscripten_run_script_string(eval.c_str());
	return std::string(result);
}

AbstractDOMElement::Ptr
EmscriptenDOMElement::parentNode()
{
	std::string eval = "Minko.tmpElement  = " + _jsAccessor + ".parentNode;";
	emscripten_run_script(eval.c_str());

	return EmscriptenDOMElement::getDOMElement("Minko.tmpElement");
}

std::list<minko::dom::AbstractDOMElement::Ptr>
EmscriptenDOMElement::childNodes()
{
	return (EmscriptenDOM::getElementList(_jsAccessor + ".childNodes"));
}

std::string
EmscriptenDOMElement::textContent()
{
	std::string eval = "(" + _jsAccessor + ".textContent)";
	char* result = emscripten_run_script_string(eval.c_str());
	return std::string(result);
}

void
EmscriptenDOMElement::textContent(std::string newTextContent)
{
	std::string eval = _jsAccessor + ".textContent = '" + newTextContent + "';";
	emscripten_run_script(eval.c_str());
}

std::string
EmscriptenDOMElement::innerHTML()
{
	std::string eval = "(" + _jsAccessor + ".innerHTML)";
	char* result = emscripten_run_script_string(eval.c_str());
	return std::string(result);
}

void
EmscriptenDOMElement::innerHTML(std::string newInnerHTML)
{
	std::string eval = _jsAccessor + ".innerHTML = '" + newInnerHTML + "';";
	emscripten_run_script(eval.c_str());
}

AbstractDOMElement::Ptr
EmscriptenDOMElement::appendChild(minko::dom::AbstractDOMElement::Ptr newChild)
{
	EmscriptenDOMElement::Ptr child = std::dynamic_pointer_cast<EmscriptenDOMElement>(newChild);
	std::string eval = _jsAccessor + ".appendChild(" + child->getJavascriptAccessor() + ");";
	emscripten_run_script(eval.c_str());

	return shared_from_this();
}

AbstractDOMElement::Ptr
EmscriptenDOMElement::removeChild(minko::dom::AbstractDOMElement::Ptr childToRemove)
{
	EmscriptenDOMElement::Ptr child = std::dynamic_pointer_cast<EmscriptenDOMElement>(childToRemove);
	std::string eval = _jsAccessor + ".removeChild(" + child->getJavascriptAccessor() + ");";
	emscripten_run_script(eval.c_str());

	return shared_from_this();
}

AbstractDOMElement::Ptr
EmscriptenDOMElement::insertBefore(minko::dom::AbstractDOMElement::Ptr newChild, minko::dom::AbstractDOMElement::Ptr adjacentNode)
{
	EmscriptenDOMElement::Ptr child = std::dynamic_pointer_cast<EmscriptenDOMElement>(newChild);
	EmscriptenDOMElement::Ptr adjNode = std::dynamic_pointer_cast<EmscriptenDOMElement>(adjacentNode);
	std::string eval = _jsAccessor + ".removeChild(" + child->getJavascriptAccessor() + ", " + adjNode->getJavascriptAccessor() + ");";
	emscripten_run_script(eval.c_str());

	return shared_from_this();
}

AbstractDOMElement::Ptr
EmscriptenDOMElement::cloneNode(bool deep)
{
	std::string eval = "Minko.tmpElement = " + _jsAccessor + ".cloneNode(" + (deep ? "true" : "false") + ");";
	emscripten_run_script(eval.c_str());

	return EmscriptenDOMElement::create("Minko.tmpElement");
}

std::string
EmscriptenDOMElement::getAttribute(std::string name)
{
	std::string eval = "(" + _jsAccessor + ".getAttribute('" + name + "'))";
	char* result = emscripten_run_script_string(eval.c_str());
	return std::string(result);
}

void
EmscriptenDOMElement::setAttribute(std::string name, std::string value)
{
	std::string eval = _jsAccessor + ".setAttribute('" + name + "', '" + value + "');";
	emscripten_run_script(eval.c_str());
}

std::list<minko::dom::AbstractDOMElement::Ptr>
EmscriptenDOMElement::getElementsByTagName(std::string tagName)
{
	return (EmscriptenDOM::getElementList(_jsAccessor + ".getElementsByTagName('" + tagName + "')"));
}


std::string
EmscriptenDOMElement::style(std::string name)
{
	std::string eval = "(" + _jsAccessor + ".style." + name + ")";
	char* result = emscripten_run_script_string(eval.c_str());
	return std::string(result);
}

void
EmscriptenDOMElement::style(std::string name, std::string value)
{
	std::string eval = _jsAccessor + ".style." + name + " = '" + value + "';";
	emscripten_run_script(eval.c_str());
}

void
EmscriptenDOMElement::addEventListener(std::string type)
{
	std::string eval = "";

	eval += "if(!" + _jsAccessor + ".minkoEvents)\n";
	eval +=	"	" + _jsAccessor + ".minkoEvents = [];\n";
	eval += _jsAccessor + ".addEventListener('" + type + "', function(event)\n";
	eval += "{\n";
	eval += "	" + _jsAccessor + ".minkoEvents.push(event);\n";
	eval += "});\n";

	emscripten_run_script(eval.c_str());
}

Signal<std::shared_ptr<AbstractDOMEvent>>::Ptr
EmscriptenDOMElement::onclick()
{
	if (!_onclickSet)
	{
		addEventListener("click");
		_onclickSet = false;
	}

	return _onclick;
}

Signal<std::shared_ptr<AbstractDOMEvent>>::Ptr
EmscriptenDOMElement::onmousedown()
{
	if (!_onmousedownSet)
	{
		addEventListener("mousedown");
		_onmousedownSet = false;
	}

	return _onmousedown;
}

Signal<std::shared_ptr<AbstractDOMEvent>>::Ptr
EmscriptenDOMElement::onmousemove()
{
	if (!_onmousemoveSet)
	{
		addEventListener("mousemove");
		_onmousemoveSet = false;
	}
	
	return _onmousemove;
}

Signal<std::shared_ptr<AbstractDOMEvent>>::Ptr
EmscriptenDOMElement::onmouseup()
{
	if (!_onmouseupSet)
	{
		addEventListener("mouseup");
		_onmouseupSet = false;
	}

	return _onmouseup;
}

Signal<std::shared_ptr<AbstractDOMEvent>>::Ptr
EmscriptenDOMElement::onmouseout()
{
	if (!_onmouseoutSet)
	{
		addEventListener("mouseout");
		_onmouseoutSet = false;
	}

	return _onmouseout;
}

Signal<std::shared_ptr<AbstractDOMEvent>>::Ptr
EmscriptenDOMElement::onmouseover()
{
	if (!_onmouseoverSet)
	{
		addEventListener("mouseover");
		_onmouseoverSet = false;
	}

	return _onmouseover;
}

void
EmscriptenDOMElement::update()
{ 
	std::string eval = "if(" + _jsAccessor + " && " + _jsAccessor + ".minkoEvents) (" + _jsAccessor + ".minkoEvents.length); else (0);";
	int l = emscripten_run_script_int(eval.c_str());

	for(int i = 0; i < l; ++i)
	{
		std::string eventName = "Minko.event" + std::to_string(_elementUid++);
		eval =  eventName + " = " + _jsAccessor + ".minkoEvents[" + std::to_string(i) + "];";
		emscripten_run_script(eval.c_str());

		EmscriptenDOMEvent::Ptr event = EmscriptenDOMEvent::create(eventName);

		std::string type = event->type();

		if (type == "click")
			_onclick->execute(event);
		else if (type == "mousedown")
			_onmousedown->execute(event);
		else if (type == "mouseup")
			_onmouseup->execute(event);
		else if (type == "mousemove")
			_onmousemove->execute(event);
		else if (type == "mouseover")
			_onmouseover->execute(event);
		else if (type == "mouseout")
			_onmouseout->execute(event);
	}

	eval = "if(" + _jsAccessor + ") " +  _jsAccessor + ".minkoEvents = [];";
	emscripten_run_script(eval.c_str());
}

#endif