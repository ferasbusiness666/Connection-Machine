#ifndef tooltip_h
#define tooltip_h

#include <RmlUi/Core.h>

typedef struct SDL_Window SDL_Window;
class SdlWindow;

class Tooltip : Rml::EventListener {
public:
	Tooltip(SDL_Window* parent, Rml::Element* element, const std::string& message);
	~Tooltip();
	void ProcessEvent(Rml::Event& event) override;
	void OnDetach(Rml::Element* element) override;

private:
	void create(Rml::Event& event);
	void destroy(Rml::Event& event);
	void move(Rml::Event& event);

	SDL_Window* parent;
	Rml::Element* element;
	std::string message;
	std::shared_ptr<SdlWindow> sdlWindow;
};
#endif /* tooltip_h */
