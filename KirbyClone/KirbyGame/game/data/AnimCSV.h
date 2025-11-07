//
// Responsibility: Parse a simple CSV and register animation clips into an Animator.
// Non-Goals:      Quoted CSV support, I/O error reporting, texture loading.
// Call-Context:   Main thread; header-only declaration with minimal deps.
//
#pragma once

namespace engine { class Animator; } // forward decl (avoid heavy include)

namespace game {

	// Load CSV and add clips to Animator. If clearExisting==true, Animator::Clear() is called first.
	// CSV formats:
	//   strip,name,sx,sy,fw,fh,count,dur,loop
	//   frame,name,sx,sy,w,h,dur,loop
	bool LoadAnimCSV ( const char* filename , engine::Animator* anim , bool clearExisting = true );

} // namespace game
