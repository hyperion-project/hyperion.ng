#ifndef EVENTENUM_H
#define EVENTENUM_H

#include <QString>

enum class Event
{
	Unknown,
	Suspend,
	Resume,
	ToggleSuspend,
	Idle,
	ResumeIdle,
	ToggleIdle,
	Reload,
	Restart,
	Quit
};

inline const char* eventToString(Event event)
{
	switch (event)
	{
	case Event::Suspend:       return "Suspend";
	case Event::Resume:        return "Resume";
	case Event::ToggleSuspend: return "ToggleSuspend";
	case Event::Idle:          return "Idle";
	case Event::Quit:          return "Quit";
	case Event::ResumeIdle:    return "ResumeIdle";
	case Event::ToggleIdle:    return "ToggleIdle";
	case Event::Reload:        return "Reload";
	case Event::Restart:       return "Restart";
	case Event::Unknown:
	default:                   return "Unknown";
	}
}

inline Event stringToEvent(const QString& event)
{
	if (event.compare("Suspend")==0)       return Event::Suspend;
	if (event.compare("Resume")==0)        return Event::Resume;
	if (event.compare("ToggleSuspend")==0) return Event::ToggleSuspend;
	if (event.compare("Idle")==0)          return Event::Idle;
	if (event.compare("Quit")==0)          return Event::Quit;
	if (event.compare("ResumeIdle")==0)    return Event::ResumeIdle;
	if (event.compare("ToggleIdle")==0)    return Event::ToggleIdle;
	if (event.compare("Reload")==0)        return Event::Reload;
	if (event.compare("Restart")==0)       return Event::Restart;
	return Event::Unknown;
}


#endif // EVENTENUM_H
