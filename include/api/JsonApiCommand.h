#ifndef JSONAPICOMMAND_H
#define JSONAPICOMMAND_H

#include <QMap>
#include <QPair>
#include <QString>

class Command {
public:
	enum Type {
		Unknown,
		Adjustment,
		Authorize,
		Clear,
		ClearAll,
		Color,
		ComponentState,
		Config,
		Correction,
		CreateEffect,
		DeleteEffect,
		Effect,
		Image,
		InputSource,
		Instance,
		LedColors,
		LedDevice,
		Logging,
		Processing,
		ServerInfo,
		Service,
		SourceSelect,
		SysInfo,
		System,
		Temperature,
		Transform,
		VideoMode
	};

	static QString toString(Type type) {
		switch (type) {
		case Adjustment: return "adjustment";
		case Authorize: return "authorize";
		case Clear: return "clear";
		case ClearAll: return "clearall";
		case Color: return "color";
		case ComponentState: return "componentstate";
		case Config: return "config";
		case Correction: return "correction";
		case CreateEffect: return "create-effect";
		case DeleteEffect: return "delete-effect";
		case Effect: return "effect";
		case Image: return "image";
		case InputSource: return "inputsource";
		case Instance: return "instance";
		case LedColors: return "ledcolors";
		case LedDevice: return "leddevice";
		case Logging: return "logging";
		case Processing: return "processing";
		case ServerInfo: return "serverinfo";
		case SourceSelect: return "sourceselect";
		case SysInfo: return "sysinfo";
		case System: return "system";
		case Temperature: return "temperature";
		case Transform: return "transform";
		case VideoMode: return "videomode";
		case Service: return "service";
		default: return "unknown";
		}
	}
};

class SubCommand {
public:
	enum Type {
		Unknown,
		Empty,
		AdminRequired,
		AddAuthorization,
		AnswerRequest,
		CreateInstance,
		CreateToken,
		DeleteInstance,
		DeleteToken,
		Discover,
		GetConfig,
		GetInfo,
		GetPendingTokenRequests,
		GetProperties,
		GetSchema,
		GetSubscriptionCommands,
		GetSubscriptions,
		GetTokenList,
		Identify,
		Idle,
		ImageStreamStart,
		ImageStreamStop,
		LedStreamStart,
		LedStreamStop,
		Login,
		Logout,
		NewPassword,
		NewPasswordRequired,
		Reload,
		RenameToken,
		RequestToken,
		Restart,
		RestoreConfig,
		Resume,
		SaveName,
		SetConfig,
		Start,
		StartInstance,
		Stop,
		StopInstance,
		Subscribe,
		Suspend,
		SwitchTo,
		ToggleIdle,
		ToggleSuspend,
		TokenRequired,
		Unsubscribe
	};

	static QString toString(Type type) {
		switch (type) {
		case Empty: return "";
		case AdminRequired: return "adminRequired";
		case AddAuthorization: return "addAuthorization";
		case AnswerRequest: return "answerRequest";
		case CreateInstance: return "createInstance";
		case CreateToken: return "createToken";
		case DeleteInstance: return "deleteInstance";
		case DeleteToken: return "deleteToken";
		case Discover: return "discover";
		case GetConfig: return "getconfig";
		case GetInfo: return "getInfo";
		case GetPendingTokenRequests: return "getPendingTokenRequests";
		case GetProperties: return "getProperties";
		case GetSchema: return "getschema";
		case GetSubscriptionCommands: return "getSubscriptionCommands";
		case GetSubscriptions: return "getSubscriptions";
		case GetTokenList: return "getTokenList";
		case Identify: return "identify";
		case Idle: return "idle";
		case ImageStreamStart: return "imagestream-start";
		case ImageStreamStop: return "imagestream-stop";
		case LedStreamStart: return "ledstream-start";
		case LedStreamStop: return "ledstream-stop";
		case Login: return "login";
		case Logout: return "logout";
		case NewPassword: return "newPassword";
		case NewPasswordRequired: return "newPasswordRequired";
		case Reload: return "reload";
		case RenameToken: return "renameToken";
		case RequestToken: return "requestToken";
		case Restart: return "restart";
		case RestoreConfig: return "restoreconfig";
		case Resume: return "resume";
		case SaveName: return "saveName";
		case SetConfig: return "setconfig";
		case Start: return "start";
		case StartInstance: return "startInstance";
		case Stop: return "stop";
		case StopInstance: return "stopInstance";
		case Subscribe: return "subscribe";
		case Suspend: return "suspend";
		case SwitchTo: return "switchTo";
		case ToggleIdle: return "toggleIdle";
		case ToggleSuspend: return "toggleSuspend";
		case TokenRequired: return "tokenRequired";
		case Unsubscribe: return "unsubscribe";
		default: return "unknown";
		}
	}
};

class Authorization {
public:
	enum Type {
		Admin,
		Yes,
		No
	};
};

class NoListenerCmd {
public:
	enum Type {
		No,
		Yes
	};
};

class InstanceCmd {
public:
	enum Type {
		No,
		Yes,
		Multi
	};
};

class JsonApiCommand {
public:

	JsonApiCommand()
		: command(Command::Unknown),
		  subCommand(SubCommand::Unknown),
		  tan(0),
		  authorization(Authorization::Admin),
		  isInstanceCmd(InstanceCmd::No),
		  isNolistenerCmd(NoListenerCmd::Yes)
	{}

	JsonApiCommand(Command::Type command, SubCommand::Type subCommand,
				   Authorization::Type authorization,
				   InstanceCmd::Type isInstanceCmd,
				   NoListenerCmd::Type isNolistenerCmd,
				   int tan = 0)
		: command(command),
		  subCommand(subCommand),
		  tan(tan),
		  authorization(authorization),
		  isInstanceCmd(isInstanceCmd),
		  isNolistenerCmd(isNolistenerCmd)
	{}

	Command::Type getCommand() const { return command; }
	SubCommand::Type getSubCommand() const { return subCommand; }
	InstanceCmd::Type instanceCmd() const { return isInstanceCmd; }
	int getTan() const { return tan; }

	QString toString() const {
		QString cmd = Command::toString(command);
		if (subCommand > SubCommand::Empty) {
			cmd += QString("-%2").arg(SubCommand::toString(subCommand));
		}
		return cmd;
	}

	Command::Type command;
	SubCommand::Type subCommand;
	int tan;

	Authorization::Type authorization;
	InstanceCmd::Type isInstanceCmd;
	NoListenerCmd::Type isNolistenerCmd;
};

typedef QMap<QPair<QString, QString>, JsonApiCommand> CommandLookupMap;

class ApiCommandRegister {
public:

	static const CommandLookupMap& getCommandLookup() {
		static const CommandLookupMap commandLookup {
			{ {"adjustment", ""}, { Command::Adjustment, SubCommand::Empty, Authorization::Yes, InstanceCmd::Multi, NoListenerCmd::Yes} },
			{ {"authorize", "adminRequired"}, { Command::Authorize, SubCommand::AdminRequired, Authorization::No, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"authorize", "answerRequest"}, { Command::Authorize, SubCommand::AnswerRequest, Authorization::Admin, InstanceCmd::No, NoListenerCmd::No} },
			{ {"authorize", "createToken"}, { Command::Authorize, SubCommand::CreateToken, Authorization::Admin, InstanceCmd::No, NoListenerCmd::No} },
			{ {"authorize", "deleteToken"}, { Command::Authorize, SubCommand::DeleteToken, Authorization::Admin, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"authorize", "getPendingTokenRequests"}, { Command::Authorize, SubCommand::GetPendingTokenRequests, Authorization::Admin, InstanceCmd::No, NoListenerCmd::No} },
			{ {"authorize", "getTokenList"}, { Command::Authorize, SubCommand::GetTokenList, Authorization::Admin, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"authorize", "login"}, { Command::Authorize, SubCommand::Login, Authorization::No, InstanceCmd::No, NoListenerCmd::No} },
			{ {"authorize", "logout"}, { Command::Authorize, SubCommand::Logout, Authorization::No, InstanceCmd::No, NoListenerCmd::No} },
			{ {"authorize", "newPassword"}, { Command::Authorize, SubCommand::NewPassword, Authorization::Admin, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"authorize", "newPasswordRequired"}, { Command::Authorize, SubCommand::NewPasswordRequired, Authorization::No, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"authorize", "renameToken"}, { Command::Authorize, SubCommand::RenameToken, Authorization::Admin, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"authorize", "requestToken"}, { Command::Authorize, SubCommand::RequestToken, Authorization::No, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"authorize", "tokenRequired"}, { Command::Authorize, SubCommand::TokenRequired, Authorization::No, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"clear", ""}, { Command::Clear, SubCommand::Empty, Authorization::Yes, InstanceCmd::Multi, NoListenerCmd::Yes} },
			{ {"clearall", ""}, { Command::ClearAll, SubCommand::Empty, Authorization::Yes, InstanceCmd::Multi, NoListenerCmd::Yes} },
			{ {"color", ""}, { Command::Color, SubCommand::Empty, Authorization::Yes, InstanceCmd::Multi, NoListenerCmd::Yes} },
			{ {"componentstate", ""}, { Command::ComponentState, SubCommand::Empty, Authorization::Yes, InstanceCmd::Multi, NoListenerCmd::Yes} },
			{ {"config", "getconfig"}, { Command::Config, SubCommand::GetConfig, Authorization::Admin, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"config", "getschema"}, { Command::Config, SubCommand::GetSchema, Authorization::Admin, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"config", "reload"}, { Command::Config, SubCommand::Reload, Authorization::Admin, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"config", "restoreconfig"}, { Command::Config, SubCommand::RestoreConfig, Authorization::Admin, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"config", "setconfig"}, { Command::Config, SubCommand::SetConfig, Authorization::Admin, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"correction", ""}, { Command::Correction, SubCommand::Empty, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"create-effect", ""}, { Command::CreateEffect, SubCommand::Empty, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"delete-effect", ""}, { Command::DeleteEffect, SubCommand::Empty, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"effect", ""}, { Command::Effect, SubCommand::Empty, Authorization::Yes, InstanceCmd::Multi, NoListenerCmd::Yes} },
			{ {"image", ""}, { Command::Image, SubCommand::Empty, Authorization::Yes, InstanceCmd::Multi, NoListenerCmd::Yes} },
			{ {"inputsource", "discover"}, { Command::InputSource, SubCommand::Discover, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"inputsource", "getProperties"}, { Command::InputSource, SubCommand::GetProperties, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"instance", "createInstance"}, { Command::Instance, SubCommand::CreateInstance, Authorization::Admin, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"instance", "deleteInstance"}, { Command::Instance, SubCommand::DeleteInstance, Authorization::Admin, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"instance", "saveName"}, { Command::Instance, SubCommand::SaveName, Authorization::Admin, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"instance", "startInstance"}, { Command::Instance, SubCommand::StartInstance, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"instance", "stopInstance"}, { Command::Instance, SubCommand::StopInstance, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"instance", "switchTo"}, { Command::Instance, SubCommand::SwitchTo, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"ledcolors", "imagestream-start"}, { Command::LedColors, SubCommand::ImageStreamStart, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"ledcolors", "imagestream-stop"}, { Command::LedColors, SubCommand::ImageStreamStop, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"ledcolors", "ledstream-start"}, { Command::LedColors, SubCommand::LedStreamStart, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"ledcolors", "ledstream-stop"}, { Command::LedColors, SubCommand::LedStreamStop, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"leddevice", "addAuthorization"}, { Command::LedDevice, SubCommand::AddAuthorization, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"leddevice", "discover"}, { Command::LedDevice, SubCommand::Discover, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"leddevice", "getProperties"}, { Command::LedDevice, SubCommand::GetProperties, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"leddevice", "identify"}, { Command::LedDevice, SubCommand::Identify, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"logging", "start"}, { Command::Logging, SubCommand::Start, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"logging", "stop"}, { Command::Logging, SubCommand::Stop, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"processing", ""}, { Command::Processing, SubCommand::Empty, Authorization::Yes, InstanceCmd::Multi, NoListenerCmd::Yes} },
			{ {"serverinfo", ""}, { Command::ServerInfo, SubCommand::Empty, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"serverinfo", "getInfo"}, { Command::ServerInfo, SubCommand::GetInfo, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"serverinfo", "subscribe"}, { Command::ServerInfo, SubCommand::Subscribe, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::No} },
			{ {"serverinfo", "unsubscribe"}, { Command::ServerInfo, SubCommand::Unsubscribe, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::No} },
			{ {"serverinfo", "getSubscriptions"}, { Command::ServerInfo, SubCommand::GetSubscriptions, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::No} },
			{ {"serverinfo", "getSubscriptionCommands"}, { Command::ServerInfo, SubCommand::GetSubscriptionCommands, Authorization::No, InstanceCmd::No, NoListenerCmd::No} },
			{ {"service", "discover"}, { Command::Service, SubCommand::Discover, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"sourceselect", ""}, { Command::SourceSelect, SubCommand::Empty, Authorization::Yes, InstanceCmd::Multi, NoListenerCmd::Yes} },
			{ {"sysinfo", ""}, { Command::SysInfo, SubCommand::Empty, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"system", "restart"}, { Command::System, SubCommand::Restart, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"system", "resume"}, { Command::System, SubCommand::Resume, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"system", "suspend"}, { Command::System, SubCommand::Suspend, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"system", "toggleSuspend"}, { Command::System, SubCommand::ToggleSuspend, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"system", "idle"}, { Command::System, SubCommand::Idle, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"system", "toggleIdle"}, { Command::System, SubCommand::ToggleIdle, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} },
			{ {"temperature", ""}, { Command::Temperature, SubCommand::Empty, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"transform", ""}, { Command::Transform, SubCommand::Empty, Authorization::Yes, InstanceCmd::Yes, NoListenerCmd::Yes} },
			{ {"videomode", ""}, { Command::VideoMode, SubCommand::Empty, Authorization::Yes, InstanceCmd::No, NoListenerCmd::Yes} }
		};
		return commandLookup;
	}

	static JsonApiCommand getCommandInfo(const QString& command, const QString& subCommand) {
		return getCommandLookup().value({command, subCommand});
	}
};

#endif // JSONAPICOMMAND_H
