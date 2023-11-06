#ifndef EVENT_H
#define EVENT_H

enum class Event
{
	Suspend,
	Resume,
	ToggleSuspend,
	Idle,
	ResumeIdle,
	ToggleIdle,
	Reload,
	Restart
};

inline const char* eventToString(Event event)
{
	switch (event)
	{
	case Event::Suspend:       return "Suspend";
	case Event::Resume:        return "Resume";
	case Event::ToggleSuspend: return "ToggleSuspend detector";
	case Event::Idle:          return "Idle";
	case Event::ResumeIdle:    return "ResumeIdle";
	case Event::ToggleIdle:    return "ToggleIdle";
	case Event::Reload:        return "Reload";
	case Event::Restart:       return "Restart";
	default:                   return "Unknown";
	}
}

#endif // EVENT_H
