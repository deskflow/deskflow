#ifndef IJOB_H
#define IJOB_H

class IJob {
  public:
	IJob() { }
	virtual ~IJob() { }

	// manipulators

	virtual void		run() = 0;
};

#endif
