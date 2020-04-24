#include "Processitem.h"
Process::Process()
{
	children() = std::vector<Process>();
};

Process::Process(const QString& name, const uint64_t& Id, const double& memoryUsage,const double& cpuUsage,const QIcon& icon,bool child)
{
    if (_name = name, _Id = Id, _memoryUsage = memoryUsage,_icon=icon,_child = child, _cpuUsage = cpuUsage) { }
}

QString Process::name() const
{
	return _name;
}

uint64_t Process::Id() const
{
	return _Id;
}

double Process::memoryUsage() const
{

	return _memoryUsage;
}

double Process::cpuUsage() const
{
    return _cpuUsage;
}

QIcon Process::icon() const
{
    return _icon;
}

bool Process::child() const
{
	return _child;
}

void Process::Setname(const QString& name)
{
	_name = name;
}

void Process::SetId(const uint64_t& Id)
{
	_Id = Id;
}

void Process::SetmemoryUsage(const double& memoryUsage)
{
	_memoryUsage = memoryUsage;
}

void Process::SetcpuUsage(const double& cpuUsage)
{
    _cpuUsage = cpuUsage;
}

void Process::UpdatecpuUsage()
{
 /*
	if constexpr (true) {
		QString  temp = "\\Process(";
		auto tempName = _name;
		temp += tempName.remove(".exe") + ")";
		temp += "\\% Processor Time";
		std::cout << temp.toStdString() << '\n';
		_Cpu = std::make_shared<PdhCPUCounter>(PdhCPUCounter{ temp.toStdString() });
	}
_cpuUsage = _Cpu->getCPUUtilization();
*/
}

void Process::SetIcon(const QIcon &icon)
{
    _icon=icon;
}

void Process::SetChild(const bool& child)
{
	_child = child;
}

void Process::addChild(Process child)
{
	_children.push_back(child);
}

std::vector<Process> Process::children()
{
	return _children;
}
