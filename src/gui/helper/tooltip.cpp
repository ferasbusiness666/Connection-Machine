#include "tooltip.h"

#include "SDL3/SDL_video.h"
#include "app.h"
#include "computerAPI/directoryManager.h"

Rml::Vector2f measureWrappedText(const Rml::String& text, Rml::Element* element, float max_width = 0) {
	if (max_width == 0) {
		max_width = element->GetParentNode()->GetBox().GetSize().x;
	}
	Rml::FontFaceHandle fontFaceHandle = Rml::GetFontEngineInterface()->GetFontFaceHandle(
		element->GetComputedValues().font_family(),
		element->GetComputedValues().font_style(),
		element->GetComputedValues().font_weight(),
		element->GetComputedValues().font_size()
	);

	if (!fontFaceHandle) return { 0, 0 };

	Rml::String current_line;
	float line_height = element->GetComputedValues().line_height().value;
	float width_of_longest_line = 0.0f;
	int line_count = 1;

	std::istringstream words(text);
	Rml::String word;

	while (words >> word) {
		if (!current_line.empty()) current_line += " ";
		current_line += word;

		float line_width = Rml::GetFontEngineInterface()->GetStringWidth(
			fontFaceHandle,
			current_line,
			Rml::TextShapingContext{ element->GetComputedValues().language(), element->GetComputedValues().direction(), element->GetComputedValues().letter_spacing() }
		);

		if (line_width <= max_width) { // word fits
			width_of_longest_line = std::max(width_of_longest_line, line_width);
		} else { // wrap
			current_line = word;
			float word_width = Rml::GetFontEngineInterface()->GetStringWidth(
				fontFaceHandle,
				current_line,
				Rml::TextShapingContext{ element->GetComputedValues().language(),
										 element->GetComputedValues().direction(),
										 element->GetComputedValues().letter_spacing() }
			);
			width_of_longest_line = std::max(width_of_longest_line, word_width);
			line_count++;
		}
	}
	return { width_of_longest_line, line_count * line_height };
}

Tooltip::Tooltip(SDL_Window* parent, Rml::Element* element, const std::string& message) : parent(parent), element(element), message(message) {
	element->AddEventListener(Rml::EventId::Mouseover, this);
	element->AddEventListener(Rml::EventId::Mousemove, this);
	element->AddEventListener(Rml::EventId::Mouseout, this);
}

Tooltip::~Tooltip() {
	element->RemoveEventListener(Rml::EventId::Mouseover, this);
	element->RemoveEventListener(Rml::EventId::Mousemove, this);
	element->RemoveEventListener(Rml::EventId::Mouseout, this);
}

void Tooltip::create(Rml::Event& event) {
	if (sdlWindow) return;
	Rml::Vector2f point(event.GetParameter<int>("mouse_x", 0), element->GetAbsoluteOffset().y + element->GetOffsetHeight()); //event.GetParameter<int>("mouse_y", 0)

	float pixelDensity = SDL_GetWindowPixelDensity(parent);
	SDL_Window* handle = SDL_CreatePopupWindow(parent, point.x / pixelDensity, point.y / pixelDensity, 200, 20, SDL_WINDOW_TOOLTIP | SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
	if (!handle) {
		logError("Failed to create tooltip {} window", "createTooltip", message);
		return;
	}

	sdlWindow = App::get().registerWindow(handle);
	WindowId windowId = MainRenderer::get().registerWindow(sdlWindow.get());
	Rml::Context* rmlContext = Rml::CreateContext(
		"Tooltip: " + message + " Window: " + std::to_string((unsigned long long)handle),
		Rml::Vector2i(sdlWindow->getSize().first, sdlWindow->getSize().second)
	);
	if (rmlContext) {
		sdlWindow->setRenderFunction([windowId, rmlContext]() {
			RmlRenderInterface* rmlRenderInterface = dynamic_cast<RmlRenderInterface*>(Rml::GetRenderInterface());
			if (rmlRenderInterface) {
				rmlContext->Update();
				rmlRenderInterface->setWindowToRenderOn(windowId);
				MainRenderer::get().prepareForRmlRender(windowId);
				rmlContext->Render();
				MainRenderer::get().endRmlRender(windowId);
			}
		});
		sdlWindow->setRecieveEventFunction([windowId, rmlContext, sdlWindow = sdlWindow](SDL_Event& event) {
			if (sdlWindow->isThisMyEvent(event)) {
				if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
					Rml::RemoveContext(rmlContext->GetName());
					MainRenderer::get().deregisterWindow(windowId);
					App::get().deregisterWindow(*sdlWindow);
					return true;
				}

				RmlSDL::InputEventHandler(rmlContext, sdlWindow->getHandle(), event, sdlWindow->getWindowScalingSize());

				// let renderer know we if resized the window
				if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
					MainRenderer::get().resizeWindow(windowId, { event.window.data1, event.window.data2 });
					rmlContext->Update();
				}
				return true;
			}
			return false;
		});
		Rml::ElementDocument* rmlDocument = rmlContext->LoadDocument(DirectoryManager::getResourceDirectory().generic_string() + "/gui/tooltip/tooltip.rml");
		Rml::Element* text = rmlDocument->GetElementById("text");
		Rml::Vector2f size = measureWrappedText(message, text);
		size.x *= 1.075f;
		size += Rml::Vector2f(
			text->GetBox().GetCumulativeEdge(Rml::BoxArea::Padding, Rml::BoxEdge::Left) + text->GetBox().GetCumulativeEdge(Rml::BoxArea::Padding, Rml::BoxEdge::Right),
			text->GetBox().GetCumulativeEdge(Rml::BoxArea::Padding, Rml::BoxEdge::Top) + text->GetBox().GetCumulativeEdge(Rml::BoxArea::Padding, Rml::BoxEdge::Bottom)
		) + Rml::Vector2f(
			text->GetParentNode()->GetBox().GetCumulativeEdge(Rml::BoxArea::Padding, Rml::BoxEdge::Left) + text->GetParentNode()->GetBox().GetCumulativeEdge(Rml::BoxArea::Padding, Rml::BoxEdge::Right),
			text->GetParentNode()->GetBox().GetCumulativeEdge(Rml::BoxArea::Padding, Rml::BoxEdge::Top) + text->GetParentNode()->GetBox().GetCumulativeEdge(Rml::BoxArea::Padding, Rml::BoxEdge::Bottom)
		);
		text->AppendChild(rmlDocument->CreateTextNode(message));
		SDL_SetWindowSize(sdlWindow->getHandle(), size.x, size.y);
		rmlDocument->Show();
	}
}

void Tooltip::destroy(Rml::Event& event) {
	if (sdlWindow) {
		sdlWindow->sendKillEvent();
		sdlWindow = nullptr;
	}
}

void Tooltip::move(Rml::Event& event) {
	if (!sdlWindow) return;
	SDL_FPoint point(event.GetParameter<int>("mouse_x", 0), event.GetParameter<int>("mouse_y", 0));

	float pixelDensity = SDL_GetWindowPixelDensity(parent);
	SDL_SetWindowPosition(sdlWindow->getHandle(), point.x / pixelDensity, point.y / pixelDensity);
}

void Tooltip::ProcessEvent(Rml::Event& event) {
	switch (event.GetId()) {
	case Rml::EventId::Mouseover: create(event); break;
	// case Rml::EventId::Mousemove: move(event); break;
	case Rml::EventId::Mouseout: destroy(event); break;
	default:;
	}
}

void Tooltip::OnDetach(Rml::Element* element) { delete this; }
