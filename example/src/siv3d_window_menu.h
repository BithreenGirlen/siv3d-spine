#ifndef SIV3D_WINDOW_MENU_H_
#define SIV3D_WINDOW_MENU_H_

#ifndef NO_S3D_USING
#define NO_S3D_USING
#endif
#include <Siv3D.hpp>

class CSiv3dWindowMenu
{
public:

	/// @brief 別のある状態に応じて有効・無効を切り替えるか否か
	using Restrictive = s3d::YesNo<struct Restrictive_tag>;
	struct ItemProprty
	{
		/// @brief 項目選択時に呼び出される関数
		std::function<void()> callback = nullptr;
		Restrictive isRestrictive = Restrictive::No;
	};

	void initialise(
		s3d::Array<std::pair<s3d::String, s3d::Array<s3d::String>>> menuItems,
		s3d::Array<s3d::Array<ItemProprty>> menuItemProperties);
	bool hasBeenInitialised() const;

	void setVisibility(bool isVisible);
	bool isVisible()const;

	void update();
	void draw();

	bool getLastItemChecked() const;
	void setLastItemChecked(bool checked);
	/// @brief Restrictive::Yes特性を有する項目の有効・無効切り替え
	void updateRestrictiveItemState(bool enabled);
private:
	std::unique_ptr<s3d::SimpleMenuBar> m_pMenuBar;
	s3d::Array<std::pair<s3d::String, s3d::Array<s3d::String>>> m_menuItems;
	s3d::Array<s3d::Array<ItemProprty>> m_menuItemProperties;

	bool m_isMenuBarVisible = true;
	s3d::MenuBarItemIndex m_lastItemIndex;
};

#endif // !SIV3D_WINDOW_MENU_H_
