

#include "siv3d_window_menu.h"

void CSiv3dWindowMenu::initialise(
	s3d::Array<std::pair<s3d::String, s3d::Array<s3d::String>>> menuItems,
	s3d::Array<s3d::Array<ItemProprty>> menuItemProperties)
{
	m_menuItems = menuItems;
	m_menuItemProperties = menuItemProperties;
	m_pMenuBar = std::make_unique<s3d::SimpleMenuBar>(m_menuItems);

	updateRestrictiveItemState(false);
}

bool CSiv3dWindowMenu::hasBeenInitialised() const
{
	return !m_menuItems.empty() && !m_menuItemProperties.empty() && m_pMenuBar != nullptr;
}

void CSiv3dWindowMenu::setVisibility(bool isVisible)
{
	m_isMenuBarVisible = isVisible;
}

bool CSiv3dWindowMenu::isVisible() const
{
	return m_isMenuBarVisible;
}

void CSiv3dWindowMenu::update()
{
	if (m_pMenuBar == nullptr)return;

	if (const auto& menuBarItem = m_pMenuBar->update())
	{
		for (size_t menuIndex = 0; menuIndex < m_menuItems.size(); ++menuIndex)
		{
			const auto& items = m_menuItems[menuIndex].second;
			for (size_t itemIndex = 0; itemIndex < items.size(); ++itemIndex)
			{
				if (menuBarItem == s3d::MenuBarItemIndex{ menuIndex, itemIndex })
				{
					if (menuIndex < m_menuItemProperties.size() && itemIndex < m_menuItemProperties[menuIndex].size())
					{
						if (m_menuItemProperties[menuIndex][itemIndex].callback != nullptr)
						{
							m_lastItemIndex = menuBarItem.value();
							m_menuItemProperties[menuIndex][itemIndex].callback();
						}
					}
				}
			}
		}
	}
}

void CSiv3dWindowMenu::draw()
{
	if (m_pMenuBar != nullptr && m_isMenuBarVisible)
	{
		m_pMenuBar->draw();
	}
}

bool CSiv3dWindowMenu::getLastItemChecked() const
{
	if (m_pMenuBar != nullptr)
	{
		return m_pMenuBar->getItemChecked(m_lastItemIndex);
	}
	return false;
}

void CSiv3dWindowMenu::setLastItemChecked(bool checked)
{
	if (m_pMenuBar != nullptr)
	{
		m_pMenuBar->setItemChecked(m_lastItemIndex, checked);
	}
}

void CSiv3dWindowMenu::updateRestrictiveItemState(bool enabled)
{
	if (m_pMenuBar != nullptr)
	{
		for (size_t menuIndex = 0; menuIndex < m_menuItemProperties.size(); ++menuIndex)
		{
			const auto& itemPropertyies = m_menuItemProperties[menuIndex];
			for (size_t itemIndex = 0; itemIndex < itemPropertyies.size(); ++itemIndex)
			{
				const auto& itemProperty = itemPropertyies[itemIndex];
				if (itemProperty.isRestrictive)
				{
					if (menuIndex < m_menuItems.size() && itemIndex < m_menuItems[menuIndex].second.size())
					{
						m_pMenuBar->setItemEnabled({ menuIndex, itemIndex }, enabled);
					}
				}
			}
		}
	}
}
