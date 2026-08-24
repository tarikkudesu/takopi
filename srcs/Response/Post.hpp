#ifndef __POST_HPP__
#define __POST_HPP__


#include "FileExplorer.hpp"

class Post
{
	private:
		Request 					&request;
		t_response_phase			&__responsePhase;

		FileExplorer				*explorer;
		Location					*location;

        std::time_t                 __startTime;
		t_multipartsection			__phase;
		BasicString					__data;
		BasicString					__form;
		std::ofstream				__fs;

		void						mpInit();
		void						mpBody();
		void						mpHeaders();
		void						processFormData();
		void						processMultiPartBody();
		void						createFile(std::vector<String> &h);
		void						writeDataIntoFile(BasicString &data);

	public:
		void						reset();
		BasicString 				&getForm();
		void						setWorkers(FileExplorer &explorer, Location &location);
		void						processData(BasicString &data);


		Post(Request &request, t_response_phase &phase);
		Post(const Post &copy);
		Post &operator=(const Post &assign);
		~Post();
};

#endif