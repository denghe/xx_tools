#pragma once
#include "pch.h"

namespace Scene_PivotEditor {

	struct Scene : xx::SceneBase {
		xx::Camera camera;

		virtual void Init() override;
		virtual void BeforeUpdate() override;
		virtual void Update() override;
		virtual void Draw() override;
	};
	inline Scene* gScene;		// init by Init()

}
