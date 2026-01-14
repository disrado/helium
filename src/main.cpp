#include "SDL3/SDL.h"

#include <print>


auto main() -> int
{
  if (auto ticks { SDL_Time{} }; SDL_GetCurrentTime(&ticks))
  {
      std::println("current time: {}", ticks);
  }
  else
  {
      std::println("failed to retrieve current time");
  }

  return 0;
}