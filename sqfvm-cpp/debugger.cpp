#include "debugger.h"
#include "netserver.h"
#include "virtualmachine.h"
#include "nlohmann\json.hpp"
#include "vmstack.h"
#include "callstack.h"
#include "sqfnamespace.h"
#include "namespaces.h"
#include "value.h"
#include "instruction.h"
#include <sstream>

namespace {
	enum srvstatus {
		NA,
		HALT,
		RUNNING,
		DONE
	};
	class srvmessage {
	public:
		virtual std::string serialize(void) = 0;
		operator std::string(void) { return serialize(); }
	};
	class errormsg : public srvmessage {
		std::string _msg;
	public:
		errormsg(std::string msg) : _msg(msg) {}
		std::string serialize(void) {
			nlohmann::json json = {
				{ "mode", "message" },
				{ "data", _msg }
			};
			return json;
		}
	};
	class statusupdate : public srvmessage {
		srvstatus _status;
	public:
		statusupdate(srvstatus status) : _status(status) {}
		std::string serialize(void) {
			nlohmann::json json;
			json["mode"] = "status";
			switch (_status)
			{
			case HALT:
				json["data"] = "HALT";
				break;
			case RUNNING:
				json["data"] = "RUNNING";
				break;
			case DONE:
				json["data"] = "DONE";
				break;
			default:
				json["data"] = "NA";
				break;
			}
			return json;
		}
	};
	class callstackmsg : public srvmessage {
		std::shared_ptr<sqf::vmstack> _stack;
	public:
		callstackmsg(std::shared_ptr<sqf::vmstack> stack) : _stack(stack) {}
		std::string serialize(void) {
			nlohmann::json data;
			int lvl = 0;
			for (auto it = _stack->stacks_begin(); it != _stack->stacks_end(); it++)
			{
				nlohmann::json jcs;
				auto cs = *it;
				jcs["lvl"] = lvl;
				jcs["scopename"] = cs->getscopename();
				jcs["namespace"] = cs->getnamespace()->get_name();
				auto variables = nlohmann::json::array();
				jcs["variables"] = variables;
				nlohmann::json options;
				if (cs->inststacksize() <= 0) {
					options["line"] = nullptr;
					options["column"] = nullptr;
					options["file"] = nullptr;
				}
				else {
					options["line"] = cs->peekinst()->line();
					options["column"] = cs->peekinst()->col();
					options["file"] = cs->peekinst()->file();
				}
				for (auto pair : cs->varmap())
				{
					variables.push_back(nlohmann::json{ { "name", pair.first },{ "value", pair.second->as_string() } });
				}
				data.push_back(jcs);
			}

			return nlohmann::json{
				{ "mode", "callstack" },
			{ "data", data }
			};
		}
	};
	class variablemsg : public srvmessage {
		std::shared_ptr<sqf::vmstack> _stack;
		nlohmann::json _data;
	public:
		variablemsg(std::shared_ptr<sqf::vmstack> stack, nlohmann::json data) : _stack(stack), _data(data) {}
		std::string serialize(void) {
			auto data = nlohmann::json::array();
			for (auto it = _data.begin(); it != _data.end(); it++)
			{
				std::string name = it->at("name");
				auto scope = it->at("scope");
				if (scope.is_number())
				{
					int numscope = scope;
					if (numscope > 0)
					{
						throw std::runtime_error("numscope has to be <= 0");
					}
					else
					{
						std::vector<std::shared_ptr<sqf::callstack>>::reverse_iterator s;
						for (s = _stack->stacks_begin(); s != _stack->stacks_end() && numscope != 0; s++, numscope++);
						if (s == _stack->stacks_end())
						{
							data.push_back(nlohmann::json{ { "name", name },{ "value", nullptr } });
						}
						else
						{
							data.push_back(nlohmann::json{ { "name", name },{ "value", (*s)->getvar(name)->as_string() } });
						}
					}
				}
				else if (scope.get<std::string>().compare("missionNamespace"))
				{
					auto ns = sqf::commands::namespaces::missionNamespace();
					data.push_back(nlohmann::json{ { "name", name },{ "value", ns->getvar(name)->as_string() } });
				}
				else if (scope.get<std::string>().compare("uiNamespace"))
				{
					auto ns = sqf::commands::namespaces::uiNamespace();
					data.push_back(nlohmann::json{ { "name", name },{ "value", ns->getvar(name)->as_string() } });
				}
				else if (scope.get<std::string>().compare("profileNamespace"))
				{
					auto ns = sqf::commands::namespaces::profileNamespace();
					data.push_back(nlohmann::json{ { "name", name },{ "value", ns->getvar(name)->as_string() } });
				}
				else if (scope.get<std::string>().compare("parsingNamespace"))
				{
					auto ns = sqf::commands::namespaces::parsingNamespace();
					data.push_back(nlohmann::json{ { "name", name },{ "value", ns->getvar(name)->as_string() } });
				}
			}
			return nlohmann::json{
				{ "mode", "variables" },
			{ "data", data }
			};
		}
	};
}


void sqf::debugger::breakmode(virtualmachine * vm)
{
	
}

void sqf::debugger::check(virtualmachine * vm)
{
	while (_server->has_message())
	{
		auto json = nlohmann::json::parse(_server->next_message());
		std::string mode = json["mode"];
		if (!mode.compare("get-callstack")) {
			_server->push_message(callstackmsg(vm->stack()));
		}
		else if (!mode.compare("control")) {
			std::string status = json["data"]["status"];
			//ToDo
		}
		else if (!mode.compare("get-variable")) {
			auto data = json["data"];
			_server->push_message(variablemsg(vm->stack(), data));
		}
		else if (!mode.compare("set-breakpoint")) {
			auto data = json["data"];
			//ToDo
		}
		else {
			std::stringstream sstream;
			sstream << "Provided mode '" << mode << "' is not known to the debugger.";
			_server->push_message(errormsg(sstream.str()));
		}
	}
}

void sqf::debugger::error(virtualmachine * vm, int line, int col, std::string file, std::string msg)
{
	_server->push_message(statusupdate(srvstatus::HALT));
	_server->push_message(errormsg(msg));
	breakmode(vm);
}

bool sqf::debugger::stop(virtualmachine * vm)
{
	if (!_server->is_accept())
		return false;
	return true;
}