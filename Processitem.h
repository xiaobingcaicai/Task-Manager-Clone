#pragma once
#include <QString>
#include <cstdint>
#include<qicon.h>
#include <windows.h>
class Process
{
protected:
	//process name
    QString _name = "";
	//process id
	uint64_t _Id = 0;

    double _memoryUsage = 0;

	double _cpuUsage = 0;
    QIcon _icon;    
	bool _child = false;
	std::vector<Process> _children;



public:
	Process();
    Process(const QString& name, const uint64_t& Id, const double& memoryUsage,const double& cpuUsage,const QIcon& icon,bool child = false);

    QString name() const;
	//process id
    uint64_t Id() const;

    double memoryUsage() const;
    double cpuUsage() const;

    QIcon icon()const;
    bool child() const;
    void Setname(const QString& name);
	void UpdatecpuUsage();
    void SetId(const uint64_t& Id);

    void SetmemoryUsage(const double& memoryUsage);

    void SetcpuUsage(const double& cpuUsage);
    void SetIcon(const QIcon& icon);
	void SetChild(const bool& child);
	void addChild(Process child);
	std::vector<Process> children();
};


