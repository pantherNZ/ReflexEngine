#pragma once

namespace Reflex
{
	enum class ResourceID : unsigned short
	{
		ArialFont,
		BackgroundTexture,
		BoardTexture,
		Egg1,
		Egg2,
		ArrowLeft,
		ArrowRight,
		SkipButton,
		EndScreen,
		HelpScreen,
		InGameScreen,
		CancelButton,
		RestartButton,
		MainMenuButton,
		PlayAgainButton,
	};
}

enum StateTypes : unsigned
{
	PentagoMenuStateType,
	PentagoGameStateType,
	SetDifficultyStateType,
	InGameMenuStateType,
	NumStateTypes,
};