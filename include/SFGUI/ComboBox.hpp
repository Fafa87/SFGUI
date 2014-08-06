#pragma once

#include <SFGUI/Bin.hpp>

#include <vector>
#include <memory>

namespace sf {
class String;
}

namespace sfg {

class Scrollbar;

/** Combobox.
 */
class SFGUI_API ComboBox : public Bin {
	public:
		typedef std::shared_ptr<ComboBox> Ptr; ///< Shared pointer.
		typedef std::shared_ptr<const ComboBox> PtrConst; ///< Shared pointer to const.
		typedef int IndexType; ///< Type for item indices.

		static const IndexType NONE; ///< Value for specifying an invalid index/no item selected.

		/** Create combo box.
		 * @return Combo box.
		 */
		static Ptr Create();

		const std::string& GetName() const override;

		/** Get selected item.
		 * @return Active item or NONE if none selected.
		 */
		IndexType GetSelectedItem() const;

		/** Get highlighted item.
		 * @return Highlighted item or NONE if none highlighted.
		 */
		IndexType GetHighlightedItem() const;

		/** Select item.
		 * @param index Item index.
		 */
		void SelectItem( IndexType index );

		/** Append item.
		 * @param text Item text.
		 */
		void AppendItem( const sf::String& text );

		/** Insert item.
		 * @param index Item index.
		 * @param text Item text.
		 */
		void InsertItem( IndexType index, const sf::String& text );

		/** Prepend item.
		 * @param text Item text.
		 */
		void PrependItem( const sf::String& text );

		/** Change item.
		 * @param index Item index.
		 * @param text Item text.
		 */
		void ChangeItem( IndexType index, const sf::String& text );

		/** Remove item.
		 * @param index Item index.
		 */
		void RemoveItem( IndexType index );

		/** Get text of selected item.
		 * @return Text of selected item or empty if none selected.
		 */
		const sf::String& GetSelectedText() const;

		/** Get item count.
		 * @return Item count.
		 */
		IndexType GetItemCount() const;

		/** Get number of items that are displayed at once when popup is opened.
		 * @return Number of items that are displayed at once when popup is opened.
		 */
		IndexType GetDisplayedItems() const;

		/** Get index of the first item to be displayed in the popup.
		 * @return Get index of the first item to be displayed in the popup.
		 */
		IndexType GetStartItemIndex() const;

		/** Get text of specific item.
		 * @param index Item index.
		 * @return Item text or empty if index is invalid.
		 */
		const sf::String& GetItem( IndexType index ) const;

		/** Is the popup being shown?
		 * @return true if the popup is being shown.
		 */
		bool IsPoppedUp() const;

		// Signals.
		static Signal::SignalID OnSelect; //!< Fired when an entry is selected.
		static Signal::SignalID OnOpen; //!< Fired when the popup is opened.

	protected:
		ComboBox();

		std::unique_ptr<RenderQueue> InvalidateImpl() const override;
		sf::Vector2f CalculateRequisition() override;

	private:
		void HandleMouseEnter( int x, int y ) override;
		void HandleMouseLeave( int x, int y ) override;
		void HandleMouseMoveEvent( int x, int y ) override;
		void HandleMouseButtonEvent( sf::Mouse::Button button, bool press, int x, int y ) override;
		void HandleStateChange( State old_state ) override;
		void HandleUpdate( float seconds ) override;
		void ChangeStartEntry();

		std::shared_ptr<Scrollbar> m_scrollbar;

		IndexType m_active_item;
		IndexType m_highlighted_item;
		std::vector<sf::String> m_entries;
		IndexType m_start_entry;
};

}
