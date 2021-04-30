////////////////////////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////////////////////////
#include <cassert>
#include <cmath>

#include <SFGUI/RichText.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <SFGUI/Context.hpp>
#include <SFGUI/RenderQueue.hpp>
#include <SFGUI/Engine.hpp>

#include <SFML/System/String.hpp>

namespace sfg
{

////////////////////////////////////////////////////////////////////////////////
void RichText::RichLine::setCharacterColor(std::size_t pos, sf::Color color)
{
    assert(pos < getLength());
    isolateCharacter(pos);
    std::size_t stringToFormat = convertLinePosToLocal(pos);
    m_texts[stringToFormat].setFillColor(color);
    updateGeometry();
}

////////////////////////////////////////////////////////////////////////////////
void RichText::RichLine::setCharacterStyle(std::size_t pos, sf::Text::Style style)
{
    assert(pos < getLength());
    isolateCharacter(pos);
    std::size_t stringToFormat = convertLinePosToLocal(pos);
    m_texts[stringToFormat].setStyle(style);
    updateGeometry();
}
////////////////////////////////////////////////////////////////////////////////
void RichText::RichLine::setCharacter(std::size_t pos, sf::Uint32 character)
{
    assert(pos < getLength());
    sf::Text& text = m_texts[convertLinePosToLocal(pos)];
    sf::String string = text.getString();
    string[pos] = character;
    text.setString(string);
    updateGeometry();
}


////////////////////////////////////////////////////////////////////////////////
void RichText::RichLine::setCharacterSize(unsigned int size)
{
    for (sf::Text &text : m_texts)
        text.setCharacterSize(size);

    updateGeometry();
}


////////////////////////////////////////////////////////////////////////////////
void RichText::RichLine::setFont(const sf::Font &font)
{
    for (sf::Text &text : m_texts)
        text.setFont(font);

    updateGeometry();
}


////////////////////////////////////////////////////////////////////////////////
std::size_t RichText::RichLine::getLength() const
{
    std::size_t count = 0;
    for (sf::Text &text : m_texts)
    {
        count += text.getString().getSize();
    }
    return count;
}


////////////////////////////////////////////////////////////////////////////////
sf::Color RichText::RichLine::getCharacterColor(std::size_t pos) const
{
    assert(pos < getLength());
    return m_texts[convertLinePosToLocal(pos)].getFillColor();
}


////////////////////////////////////////////////////////////////////////////////
sf::Uint32 RichText::RichLine::getCharacterStyle(std::size_t pos) const
{
    assert(pos < getLength());
    return m_texts[convertLinePosToLocal(pos)].getStyle();
}


////////////////////////////////////////////////////////////////////////////////
sf::Uint32 RichText::RichLine::getCharacter(std::size_t pos) const
{
    assert(pos < getLength());
    sf::Text& text = m_texts[convertLinePosToLocal(pos)];
    return text.getString()[pos];
}


////////////////////////////////////////////////////////////////////////////////
const std::vector<sf::Text> &RichText::RichLine::getTexts() const
{
    return m_texts;
}

////////////////////////////////////////////////////////////////////////////////
void RichText::RichLine::appendText(sf::Text text)
{
    updateTextAndGeometry(text);
    m_texts.push_back(std::move(text));
}


////////////////////////////////////////////////////////////////////////////////
sf::FloatRect RichText::RichLine::getLocalBounds() const
{
    return m_bounds;
}


////////////////////////////////////////////////////////////////////////////////
sf::FloatRect RichText::RichLine::getGlobalBounds() const
{
    return getTransform().transformRect(getLocalBounds());
}


////////////////////////////////////////////////////////////////////////////////
void RichText::RichLine::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
    states.transform *= getTransform();

    for (const sf::Text &text : m_texts)
        target.draw(text, states);
}


////////////////////////////////////////////////////////////////////////////////
std::size_t RichText::RichLine::convertLinePosToLocal(std::size_t& pos) const
{
    assert(pos < getLength());
    std::size_t arrayIndex = 0;
    for (; pos >= m_texts[arrayIndex].getString().getSize(); ++arrayIndex)
    {
        pos -= m_texts[arrayIndex].getString().getSize();
    }
    return arrayIndex;
}


////////////////////////////////////////////////////////////////////////////////
void RichText::RichLine::isolateCharacter(std::size_t pos)
{
    std::size_t localPos = pos;
    std::size_t index = convertLinePosToLocal(localPos);
    sf::Text temp = m_texts[index];

    sf::String string = temp.getString();
    if (string.getSize() == 1)
        return;

    m_texts.erase(m_texts.begin() + index);
    if (localPos != string.getSize() - 1)
    {
        temp.setString(string.substring(localPos+1));
        m_texts.insert(m_texts.begin() + index, temp);
    }

    temp.setString(string.substring(localPos, 1));
    m_texts.insert(m_texts.begin() + index, temp);
    
    if (localPos != 0)
    {
        temp.setString(string.substring(0, localPos));
        m_texts.insert(m_texts.begin() + index, temp);
    }
}


////////////////////////////////////////////////////////////////////////////////
void RichText::RichLine::updateGeometry() const
{
    m_bounds = sf::FloatRect();

    for (sf::Text& text : m_texts)
        updateTextAndGeometry(text);
}

////////////////////////////////////////////////////////////////////////////////
void RichText::RichLine::updateTop() const
{
    for (sf::Text& text : m_texts)
    {
        text.setPosition(text.getPosition().x, this->getPosition().y);
    }
}


////////////////////////////////////////////////////////////////////////////////
void RichText::RichLine::updateTextAndGeometry(sf::Text& text) const
{
    // Set text offset
    text.setPosition(m_bounds.width, 0.f);

    // Update bounds
    auto character_size = text.getCharacterSize();
    auto line_spacing = text.getFont()->getLineSpacing(character_size);
    float lineSpacing = std::floor(line_spacing);
    m_bounds.height = std::max(m_bounds.height, lineSpacing);
    m_bounds.width += text.getGlobalBounds().width;
}


////////////////////////////////////////////////////////////////////////////////
RichText::RichText()
    : RichText(nullptr)
{

}


////////////////////////////////////////////////////////////////////////////////
RichText::RichText(const sf::Font& font)
    : RichText(&font)
{

}


////////////////////////////////////////////////////////////////////////////////
RichText::Ptr RichText::Create(const sf::String& text) {
    Ptr label(new RichText());
    *label << text;
    label->RequestResize();
    return label;
}


////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<RenderQueue> RichText::InvalidateImpl() const {
    return Context::Get().GetEngine().CreateRichTextDrawable(std::dynamic_pointer_cast<const RichText>(shared_from_this()));
}


sf::Vector2f RichText::CalculateRequisition() {
    auto tmp_bounds = sf::FloatRect();

    for (RichLine &line : m_lines) {
        line.setPosition(0.f, tmp_bounds.height);

        tmp_bounds.height += line.getGlobalBounds().height;
        tmp_bounds.width = std::max(tmp_bounds.width, line.getGlobalBounds().width);
    }

    return sf::Vector2f(tmp_bounds.width, tmp_bounds.height);
}


void RichText::HandleRequisitionChange() {
    static auto calculate_y_requisition = false;

    if (!calculate_y_requisition) {
        calculate_y_requisition = true;
        RequestResize();
    }
    else {
        calculate_y_requisition = false;
    }
}

void RichText::HandleSizeChange() {
    static auto calculate_y_requisition = false;

    if (!calculate_y_requisition) {
        calculate_y_requisition = true;
        RequestResize();
    }
    else {
        calculate_y_requisition = false;
    }
}




////////////////////////////////////////////////////////////////////////////////
const std::string& RichText::GetName() const {
    static const std::string name("RichText");
    return name;
}

////////////////////////////////////////////////////////////////////////////////
RichText& RichText::operator << (const TextStroke& stroke)
{
    m_currentStroke = stroke;
    return *this;
}

RichText& RichText::operator << (const Outline& outline)
{
    m_currentStroke.outline = outline.outline;
    m_currentStroke.thickness = outline.thickness;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////
RichText & RichText::operator << (const sf::Color& color)
{
    m_currentStroke.fill = color;
    return *this;
}


////////////////////////////////////////////////////////////////////////////////
RichText & RichText::operator << (sf::Text::Style style)
{
    m_currentStyle = style;
    return *this;
}


////////////////////////////////////////////////////////////////////////////////
std::vector<sf::String> explode(const sf::String& string, sf::Uint32 delimiter)
{
    if (string.isEmpty())
        return std::vector<sf::String>();

    // For each character in the string
    std::vector<sf::String> result;
    sf::String buffer;
    for (sf::Uint32 character : string) {
        // If we've hit the delimiter character
        if (character == delimiter) {
            // Add them to the result vector
            result.push_back(buffer);
            buffer.clear();
        } else {
            // Accumulate the next character into the sequence
            buffer += character;
        }
    }

    // Add to the result if buffer still has contents or if the last character is a delimiter
    if (!buffer.isEmpty() || string[string.getSize() - 1] == delimiter)
        result.push_back(buffer);

    return result;
}


////////////////////////////////////////////////////////////////////////////////
RichText & RichText::operator << (const sf::String& string)
{
    // Maybe skip
    if (string.isEmpty())
        return *this;

    // Explode into substrings
    std::vector<sf::String> subStrings = explode(string, '\n');

    // Append first substring using the last line
    auto it = subStrings.begin();
    if (it != subStrings.end()) {
        // If there isn't any line, just create it
        if (m_lines.empty())
            m_lines.resize(1);

        // Remove last line's height
        RichLine &line = m_lines.back();
        m_bounds.height -= line.getGlobalBounds().height;

        // Append text
        line.appendText(createText(*it));

        // Update bounds
        m_bounds.height += line.getGlobalBounds().height;
        m_bounds.width = std::max(m_bounds.width, line.getGlobalBounds().width);
    }

    // Append the rest of substrings as new lines
    while (++it != subStrings.end()) {
        RichLine line;
        line.setPosition(0.f, m_bounds.height);
        line.appendText(createText(*it));
        m_lines.push_back(std::move(line));

        // Update bounds
        m_bounds.height += line.getGlobalBounds().height;
        m_bounds.width = std::max(m_bounds.width, line.getGlobalBounds().width);
    }

    updateGeometry();
    // Return
    return *this;
}


////////////////////////////////////////////////////////////////////////////////
void RichText::setCharacterColor(std::size_t line, std::size_t pos, sf::Color color)
{
    assert(line < m_lines.size());
    m_lines[line].setCharacterColor(pos, color);
    updateGeometry();
}


////////////////////////////////////////////////////////////////////////////////
void RichText::setCharacterStyle(std::size_t line, std::size_t pos, sf::Text::Style style)
{
    assert(line < m_lines.size());
    m_lines[line].setCharacterStyle(pos, style);
    updateGeometry();
}


////////////////////////////////////////////////////////////////////////////////
void RichText::setCharacter(std::size_t line, std::size_t pos, sf::Uint32 character)
{
    assert(line < m_lines.size());
    m_lines[line].setCharacter(pos, character);
    updateGeometry();
}

////////////////////////////////////////////////////////////////////////////////
void RichText::setCharacterSize(unsigned int size)
{
    // Maybe skip
    if (m_characterSize == size)
        return;

    // Update character size
    m_characterSize = size;

    // Set texts character size
    for (RichLine &line : m_lines)
        line.setCharacterSize(size);

    updateGeometry();
}


////////////////////////////////////////////////////////////////////////////////
void RichText::setFont(const sf::Font& font)
{
    // Maybe skip
    if (m_font == &font)
        return;

    // Update font
    m_font = &font;

    // Set texts font
    for (RichLine &line : m_lines)
        line.setFont(font);

    updateGeometry();
}


////////////////////////////////////////////////////////////////////////////////
void RichText::clear()
{
    // Clear texts
    m_lines.clear();

    // Reset bounds
    m_bounds = sf::FloatRect();
}


////////////////////////////////////////////////////////////////////////////////
sf::Color RichText::getCharacterColor(std::size_t line, std::size_t pos) const
{
    assert(line < m_lines.size());
    return m_lines[line].getCharacterColor(pos);
}


////////////////////////////////////////////////////////////////////////////////
sf::Uint32 RichText::getCharacterStyle(std::size_t line, std::size_t pos) const
{
    assert(line < m_lines.size());
    return m_lines[line].getCharacterStyle(pos);
}


////////////////////////////////////////////////////////////////////////////////
sf::Uint32 RichText::getCharacter(std::size_t line, std::size_t pos) const
{
    assert(line < m_lines.size());
    return m_lines[line].getCharacter(pos);
}


////////////////////////////////////////////////////////////////////////////////
const std::vector<RichText::RichLine> &RichText::getLines() const
{
    return m_lines;
}


////////////////////////////////////////////////////////////////////////////////
unsigned int RichText::getCharacterSize() const
{
    return m_characterSize;
}


////////////////////////////////////////////////////////////////////////////////
const sf::Font *RichText::getFont() const
{
    return m_font;
}


////////////////////////////////////////////////////////////
sf::FloatRect RichText::getLocalBounds() const
{
    return m_bounds;
}


////////////////////////////////////////////////////////////
sf::FloatRect RichText::getGlobalBounds() const
{
    return getTransform().transformRect(getLocalBounds());
}


////////////////////////////////////////////////////////////////////////////////
void RichText::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();

    for (const RichLine &line : m_lines)
        target.draw(line, states);
}


////////////////////////////////////////////////////////////////////////////////
RichText::RichText(const sf::Font* font)
    : m_font(font),
      m_characterSize(30),
      m_currentStroke{ sf::Color::White, sf::Color::Transparent },
      m_currentStyle(sf::Text::Regular)
{

}


////////////////////////////////////////////////////////////////////////////////
sf::Text RichText::createText(const sf::String& string) const
{
    sf::Text text;
    text.setString(string);
    text.setFillColor(m_currentStroke.fill);
    text.setOutlineColor(m_currentStroke.outline);
    text.setOutlineThickness(m_currentStroke.thickness);
    text.setStyle(m_currentStyle);
    text.setCharacterSize(m_characterSize);
    if (m_font)
        text.setFont(*m_font);

    return text;
}


////////////////////////////////////////////////////////////////////////////////
void RichText::updateGeometry()
{
    m_bounds = sf::FloatRect();

    for (RichLine &line : m_lines) {
        line.setPosition(0.f, m_bounds.height);

        m_bounds.height += line.getGlobalBounds().height;
        m_bounds.width = std::max(m_bounds.width, line.getGlobalBounds().width);
    }
    RequestResize();
    Invalidate();
}

}
