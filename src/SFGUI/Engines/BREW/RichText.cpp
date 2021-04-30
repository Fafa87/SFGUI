#include <SFGUI/Engines/BREW.hpp>
#include <SFGUI/Renderer.hpp>
#include <SFGUI/RichText.hpp>
#include <SFGUI/RenderQueue.hpp>

#include <SFML/Graphics/Text.hpp>

namespace sfg {
    namespace eng {
        std::unique_ptr<RenderQueue> BREW::CreateRichTextDrawable(std::shared_ptr<const RichText> richtext) const {
            std::unique_ptr<RenderQueue> queue(new RenderQueue);

            for (const auto &line : richtext->getLines())
            {
                float pos_line_y = line.getPosition().y;
                line.updateTop();
                for (auto &text : line.getTexts())
                {
                    queue->Add(Renderer::Get().CreateText(text));
                }
            }

            return queue;
        }

    }
}
