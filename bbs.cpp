#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <stdexcept>
#include <vector>

//#include <pthread.h>
#include <sys/time.h>

class Comment {
public:
	timeval tv;
	std::string name;
	std::string mail;
	std::string comment;
};

class Comments {
private:
	//pthread_mutex_t mutex;
	std::vector<Comment*> *list;
public:
	timeval last_add;
	timeval last_load;
	timeval last_save;
	unsigned long id;
	std::string subject;
	Comments();
	Comments(const unsigned long i);
	Comments(const unsigned long i, const std::string s);
	virtual ~Comments();
	void add(Comment *c);
	void load();
	void save();
	bool operator < (const Comments &c);
};

class BBS {
private:
	std::string dir;
	std::list<Comments*> *list;
public:
	BBS(const char *d);
	virtual ~BBS();
	void load();
	void save();
};

bool strtotime(timeval *tv, const char *str);
bool timetostr(char *str, const size_t size, const timeval tv);

Comments::Comments() {
	id = 0;
	timerclear(&last_add);
	timerclear(&last_load);
	timerclear(&last_save);
	list = new std::vector<Comment*>();
	//pthread_mutex_init(&mutex, NULL);
}

Comments::Comments(const unsigned long i) {
	id = i;
	timerclear(&last_add);
	timerclear(&last_load);
	timerclear(&last_save);
	list = new std::vector<Comment*>();
	//pthread_mutex_init(&mutex, NULL);
}

Comments::Comments(const unsigned long i, const std::string s) {
	id = i;
	subject.assign(s);
	timerclear(&last_add);
	timerclear(&last_load);
	timerclear(&last_save);
	list = new std::vector<Comment*>();
	//pthread_mutex_init(&mutex, NULL);
}

Comments::~Comments() {
	//pthread_mutex_lock(&mutex);
	if (list != 0) {
		std::vector<Comment*>::iterator it;
		for (it = list->begin(); it != list->end(); ++it){
			delete *it;
		}
		delete list;
	}
	//pthread_mutex_unlock(&mutex);
	//pthread_mutex_destroy(&mutex, NULL);
}

void Comments::add(Comment *c) {
	//pthread_mutex_lock(&mutex);
	list->push_back(c);
	gettimeofday(&last_add, NULL);
	//pthread_mutex_unlock(&mutex);
}

void Comments::load() {
	//pthread_mutex_lock(&mutex);
	std::ifstream fin;
	std::ostringstream path;
	path << id << ".log";
	fin.open(path.str().c_str(), std::ifstream::in);
	if (fin.is_open()) {
		timeval tv;
		timerclear(&tv);
		std::string name;
		std::string mail;
		std::string line;
		std::vector<std::string> lines;
		if (std::getline(fin, line)) {
			if (line.compare(0, 2, "$$") == 0) {
				if (line.compare(2, 9, "Subject: ") == 0) {
					subject.assign(line, 11, std::string::npos);
				}
			}
		}
		while (std::getline(fin, line)) {
			if (line.compare(0, 2, "$$") == 0) {
				if (line.compare(2, 6, "Time: ") == 0) {
					if (line.length() == 34) { 
						char buf[27];
						size_t len = line.copy(buf, 26, 8);
						buf[len] = '\0';
						strtotime(&tv, buf);
					}
				}
				if (line.compare(2, 6, "Name: ") == 0) {
					name.assign(line, 8, std::string::npos);
					continue;
				}
				if (line.compare(2, 6, "Mail: ") == 0) {
					mail.assign(line, 8, std::string::npos);
					continue;
				}
				if (line.compare(2, 2, "--") == 0) {
					Comment *c = new Comment();
					c->tv = tv;
					c->name.assign(name);
					c->mail.assign(mail);
					if (! lines.empty()) {
						std::ostringstream comment;
						std::vector<std::string>::iterator it;
						for (it = lines.begin(); it != lines.end(); ++it) {
							comment << *it << std::endl;
						}
						c->comment.assign(comment.str());
						lines.clear();
					}
					list->push_back(c);
					timerclear(&tv);
					name.clear();
					mail.clear();
					continue;
				}
				if (line.compare(2, 2, "$$") == 0) {
					lines.push_back(line.substr(2));
					continue;
				}
			} else {
				lines.push_back(line);
			}
		}
	}
	fin.close();
	if (! list->empty()) {
		last_add = list->back()->tv;
	}
	gettimeofday(&last_load, NULL);
	//pthread_mutex_unlock(&mutex);
}

void Comments::save() {
	//pthread_mutex_lock(&mutex);
	std::ofstream fout;
	std::ostringstream path;
	path << id << ".log";
	fout.open(path.str().c_str(), std::ofstream::out);
	if (fout.is_open()) {
		fout << "$$Subject: " << subject << std::endl;
		std::vector<Comment*>::iterator it;
		for (it = list->begin(); it != list->end(); ++it) {
			char buf[27];
			if (timetostr(buf, sizeof(buf), (*it)->tv))
				fout << "$$Time: " << buf << std::endl;
			if (! (*it)->name.empty())
				fout << "$$Name: " << (*it)->name << std::endl;
			if (! (*it)->mail.empty())
				fout << "$$Mail: " << (*it)->mail << std::endl;
			if (! (*it)->comment.empty()) {
				std::string line;
				std::istringstream strin((*it)->comment);
				while (getline(strin, line)) {
					if (line.compare(0, 2, "$$") == 0) {
						fout << "$$" << line << std::endl;
					} else {
						fout << line << std::endl;
					}
				}
			}
			fout << "$$--" << std::endl;
		}
	}
	fout.close();
	gettimeofday(&last_save, NULL);
	//pthread_mutex_unlock(&mutex);
}

bool Comments::operator < (const Comments &c) {
	return timercmp(&last_add, &c.last_add, >) != 0;
}

BBS::BBS(const char *d) {
	dir.assign(d);
	list = new std::list<Comments*>();
}

BBS::~BBS() {
	std::list<Comments*>::iterator it;
	for (it = list->begin(); it != list->end(); ++it) {
		delete *it;
	}
	delete list;
}

void BBS::load() {
	std::ifstream fin;
	std::ostringstream path;
	path << dir << "/index.log";
	fin.open(path.str().c_str(), std::ifstream::in);
	if (fin.is_open()) {
		std::string line;
		while (std::getline(fin, line)) {
			size_t pos = line.find(':');
			if (pos != std::string::npos) {
				unsigned long id = atol(line.substr(0, pos).c_str());
				if (id > 0) {
					list->push_back(new Comments(id, line.substr(pos + 1)));
				}
			} else {
				unsigned long id = atol(line.c_str());
				if (id > 0) {
					list->push_back(new Comments(id));
				}
			}
		}
	}
	fin.close();
}

void BBS::save() {
	std::ofstream fout;
	std::ostringstream path;
	path << dir << "/index.log";
	fout.open(path.str().c_str(), std::ofstream::out);
	if (fout.is_open()) {
		std::list<Comments*>::iterator it;
		for (it = list->begin(); it != list->end(); ++it) {
			fout << (*it)->id << ':' << (*it)->subject << std::endl;
		}
	}
	fout.close();
}

bool strtotime(timeval *tv, const char *str) {
	int i;
	long l, sec, usec;
	char buf[7];
	std::tm tt;
	if (str == 0) return false;
	if (std::strlen(str) != 26) return false;
	std::strncpy(buf, str +  0, 4); buf[4] = '\0'; i = std::atoi(buf); if (i >= 1900) tt.tm_year = i - 1900;
	std::strncpy(buf, str +  5, 2); buf[2] = '\0'; i = std::atoi(buf); if (1 <= i && i <= 12) tt.tm_mon = i - 1;
	std::strncpy(buf, str +  8, 2); buf[2] = '\0'; i = std::atoi(buf); if (i >= 1) tt.tm_mday = i;
	std::strncpy(buf, str + 11, 2); buf[2] = '\0'; i = std::atoi(buf); if (i >= 0) tt.tm_hour = i;
	std::strncpy(buf, str + 14, 2); buf[2] = '\0'; i = std::atoi(buf); if (i >= 0) tt.tm_min = i;
	std::strncpy(buf, str + 17, 2); buf[2] = '\0'; i = std::atoi(buf); if (i >= 0) tt.tm_sec = i;
	std::strncpy(buf, str + 20, 6); buf[6] = '\0'; l = std::atol(buf); if (l >= 0) usec = l;
	sec = std::mktime(&tt);
	if (sec != -1) {
		tv->tv_sec = sec;
		tv->tv_usec = usec;
		return true;
	}
	return false;
}

bool timetostr(char *str, const size_t size, const timeval tv) {
	if (str == 0) return false;
	char buf1[20];
	char buf2[8];
	std::time_t t = tv.tv_sec;
	std::tm *tt = std::localtime(&t);
	std::strftime(buf1, sizeof(buf1), "%Y-%m-%d %H:%M:%S", tt);
	std::sprintf(buf2, ".%06ld", tv.tv_usec);
	if (size >= sizeof(buf1) + sizeof(buf2) - 1) {
		strcpy(str, buf1);
		strcat(str, buf2);
		return true;
	}
	return false;
}

int main(int argc, char **argv) {
	BBS bbs(".");
	return 0;
}
