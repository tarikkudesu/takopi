#ifndef __GET_HPP__
#define __GET_HPP__

#include "Post.hpp"

class Get
{
	private:
		Request							&request;
		t_response_phase				&__responsePhase;
        
		FileExplorer				    *explorer;
		Location						*location;

		size_t							__bodySize;
		t_get_file_operation			__phase;
		std::ifstream					__file;

		void							getInPhase();
		void							duringGetPhase(BasicString &body);

	public:

		void							reset();
		void							setWorkers(FileExplorer &explorer, Location &location);
		void							executeGet(BasicString &body);

		Get(Request &request, t_response_phase &phase);
		Get(const Get &copy);
		Get &operator=(const Get &assign);
		~Get();
};

#endif