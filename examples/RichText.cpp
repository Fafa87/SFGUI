// Always include the necessary header files.
// Including SFGUI/Widgets.hpp includes everything
// you can possibly need automatically.

// Always include the necessary header files.
// Including SFGUI/Widgets.hpp includes everything
// you can possibly need automatically.
#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>

#include <SFML/Graphics.hpp>


int main()
{
    // Create the main SFML window
    sf::RenderWindow app_window(sf::VideoMode(800, 600), "SFGUI RichText Example", sf::Style::Titlebar | sf::Style::Close);

    // We have to do this because we don't use SFML to draw.
    app_window.resetGLStates();

    // Create an SFGUI. This is required before doing anything with SFGUI.
    sfg::SFGUI sfgui;

    // Create our main SFGUI window
    auto window = sfg::Window::Create();
    window->SetTitle("Title");

    sf::Font font;
    // Load it from a file
    if (!font.loadFromFile("D:\\Fafa\\Domki\\SFGUI\\SFGUI++\\build\\examples\\Debug\\sansation.ttf"))
    {
        return -1;
        // error...
    }

    // Create the richtext.
    auto richtext = sfg::RichText::Create();
    richtext->setFont(font);

    // Set the text of the richtext.
    *richtext << "Default colors. " << sf::Text::Bold << sf::Color::Cyan << "This "
        << sf::Text::Italic << sf::Color::White << "is\nan\n"
        << sf::Text::Regular << sf::Color::Green << "example" << " going now very long and booooring"
        << sf::Color::White << ".\n"
        << sf::Text::Underlined << "It looks good!\n" << sf::Text::StrikeThrough
        << sfg::Outline{ sf::Color::Blue, 3.f } << "Really good!"
        << sf::Text::Italic << sf::Color::White << "start chatting below...\n";

    // Create the ScrolledWindow.
    auto scrolledwindow = sfg::ScrolledWindow::Create();
    scrolledwindow->SetScrollbarPolicy(sfg::ScrolledWindow::VERTICAL_AUTOMATIC | sfg::ScrolledWindow::HORIZONTAL_AUTOMATIC);
    scrolledwindow->AddWithViewport(richtext);
    scrolledwindow->SetRequisition(sf::Vector2f(50.f, 50.f));
    scrolledwindow->SetStick(sfg::ScrolledWindow::ScrollbarSticking::YES);

    // Create label.
    auto label = sfg::Label::Create("Type chat and enter:");
    label->SetAlignment(sf::Vector2f(0,0));

    // Create the Entry for adding text to richtext.
    auto entry = sfg::Entry::Create("");
    entry->GetSignal(sfg::Entry::OnReturnPressed).Connect([&richtext, &entry] {
        *richtext << sf::Text::Bold << sf::Color::Green << "Fafa87:" << sf::Text::Regular << sf::Color::White << entry->GetText() << "\n";
        entry->SetText("");
    });
    entry->SetRequisition(sf::Vector2f(80.f, 20.f));

    // Pack both into a box.
    auto box = sfg::Box::Create(sfg::Box::Orientation::VERTICAL);
    box->SetSpacing(10);
    box->Pack(scrolledwindow);
    box->Pack(sfg::Separator::Create(), false);
    box->Pack(label, false);
    box->Pack(entry, false);

    window->Add(box);

    // Start the game loop
    while (app_window.isOpen()) {
        // Process events
        sf::Event event;

        while (app_window.pollEvent(event)) {
            // Handle events
            window->HandleEvent(event);

            // Close window : exit
            if (event.type == sf::Event::Closed) {
                return EXIT_SUCCESS;
            }
        }

        // Update the GUI, note that you shouldn't normally
        // pass 0 seconds to the update method.
        window->Update(0.f);

        // Clear screen
        app_window.clear();

        // Draw the GUI
        sfgui.Display(app_window);

        // Update the window
        app_window.display();
    }

    return EXIT_SUCCESS;
}