#include <SFGUI/Window.hpp>
#include <SFGUI/RenderEngine.hpp>
#include <SFGUI/Context.hpp>
#include <iostream>

namespace sfg {

Window::Window() :
	Bin(),
	m_skipreallocation( false )
{
	OnAdd.Connect( &Window::HandleAdd, this );
	OnSizeAllocate.Connect( &Window::HandleSizeAllocate, this );
}

Window::Ptr Window::Create() {
	Window::Ptr  window( new Window );

	// Allocate minimum size.
	window->AllocateSize( sf::FloatRect( 0, 0, 1, 1 ) );

	return window;
}

sf::Drawable* Window::InvalidateImpl() {
	return Context::Get().GetRenderEngine().CreateWindowDrawable( boost::shared_dynamic_cast<Window>( shared_from_this() ) );
}

void Window::SetTitle( const sf::String& title ) {
	m_title = title;
	Invalidate();
}

const sf::String& Window::GetTitle() const {
	return m_title;
}

sf::FloatRect Window::GetClientRect() const {
	sf::FloatRect  clientrect( GetAllocation() );
	float  title_height( Context::Get().GetRenderEngine().GetProperty( "Window.title-height", 15.f ) );
	float  border_width( Context::Get().GetRenderEngine().GetProperty( "Window.border-width", 2.f ) );

	clientrect.Left += border_width + GetBorderWidth();
	clientrect.Top += title_height + border_width + GetBorderWidth();
	clientrect.Width -= 2 * border_width + 2 * GetBorderWidth();
	clientrect.Height -= title_height + 2 * border_width + 2 * GetBorderWidth();

	return clientrect;
}

void Window::HandleAdd( Widget::Ptr /*widget*/, Widget::Ptr child ) {
	QueueResize( child );
}

void Window::QueueResize( Widget::Ptr widget ) {
	if( widget != shared_from_this() && !IsChild( widget ) ) {
		return;
	}

	float  border_width( Context::Get().GetRenderEngine().GetProperty( "Window.border-width", 2.f ) );
	float  title_height( Context::Get().GetRenderEngine().GetProperty( "Window.title-height", 15.f ) );

	RequestSize(
		sf::Vector2f(
			widget->GetRequisition().x + 2 * border_width + 2 * GetBorderWidth(),
			widget->GetRequisition().y + 2 * border_width + title_height + 2 * GetBorderWidth()
		)
	);

	// Allocate size if current is smaller than requisition.
	if( GetRequisition().x > GetAllocation().Width || GetRequisition().y > GetAllocation().Height ) {
		m_skipreallocation = true;
		AllocateSize(
			sf::FloatRect(
				GetAllocation().Left,
				GetAllocation().Top,
				std::max( GetRequisition().x, GetAllocation().Width ),
				std::max( GetRequisition().y, GetAllocation().Height )
			)
		);
	}

	// Make sure child is in the client area.
	widget->AllocateSize(
		sf::FloatRect(
			GetAllocation().Left + border_width + GetBorderWidth(),
			GetAllocation().Top + border_width + title_height + GetBorderWidth(),
			GetAllocation().Width - 2 * border_width - 2 * GetBorderWidth(),
			GetAllocation().Height - 2 * border_width - 2 * GetBorderWidth() - title_height
		)
	);
}

void Window::HandleSizeAllocate( Widget::Ptr /*widget*/, const sf::FloatRect& /*oldallocation*/ ) {
	if( !GetChild() || m_skipreallocation ) {
		m_skipreallocation = false;
		return;
	}

	// This is only called when the window's allocation has been changed from the
	// outside, i.e. not requested by a child.
	GetChild()->AllocateSize( GetClientRect() );
}

}
