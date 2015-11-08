#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <Rcpp.h>
#include <set>

// forward declarations
class Arrival;

/** 
 *  Base class.
 */
class Activity {
public:
  std::string name;
  std::string resource;
  int n;
  
  Activity(std::string name, std::string resource): 
    name(name), resource(resource), n(1), ptr(NULL) {}
  virtual ~Activity(){}
  
  virtual void show(int indent=0) {
    for (int i = 0; i < indent; ++i)
      Rcpp::Rcout << " ";
    Rcpp::Rcout << "{ Activity: " << name << "(" << resource << ") | ";
  }
  
  virtual double run(Arrival* arrival) = 0;
  virtual Activity* get_next() { return ptr; }
  virtual void set_next(Activity* activity) { ptr = activity; }
  
private:
  Activity* ptr;
};

/**
 * Seize a resource.
 */
class Seize: public Activity {
public:
  Seize(std::string resource, int amount):
    Activity("Seize", resource), amount(amount) {}
  
  void show(int indent=0) {
    Activity::show(indent);
    Rcpp::Rcout << "amount: " << amount << " }" << std::endl;
  }
  
  double run(Arrival* arrival);
  
private:
  int amount;
};

/**
 * Release a resource.
 */
class Release: public Activity {
public:
  Release(std::string resource, int amount):
    Activity("Release", resource), amount(amount) {}
  
  void show(int indent=0) {
    Activity::show(indent);
    Rcpp::Rcout << "amount: " << amount << " }" << std::endl;
  }
  
  double run(Arrival* arrival);
  
private:
  int amount;
};

/**
 * Timeout.
 */
class Timeout: public Activity {
public:
  Timeout(Rcpp::Function duration):
    Activity("Timeout", "none"), duration(duration) {}
  
  void show(int indent=0) {
    Activity::show(indent);
    Rcpp::Rcout << "duration: function() }" << std::endl;
  }
  
  double run(Arrival* arrival) {
    return fabs(Rcpp::as<double>(duration()));
  }
  
private:
  Rcpp::Function duration;
};

/**
 * Timeout.
 */
class Branch: public Activity {
public:
  Branch(Rcpp::Function option, std::vector<bool> merge, std::vector<Rcpp::Environment> trj):
    Activity("Branch", "none"), option(option), merge(merge), trj(trj), selected(NULL) {
    n = 0;
    for (unsigned int i = 0; i < trj.size(); i++) {
      Rcpp::Function get_head(trj[i]["get_head"]);
      path.push_back(Rcpp::as<Rcpp::XPtr<Activity> >(get_head()));
      if (merge[i]) {
        Rcpp::Function get_tail(trj[i]["get_tail"]);
        Rcpp::XPtr<Activity> tail(get_tail());
        tail->set_next(this);
      }
      Rcpp::Function get_n_activities(trj[i]["get_n_activities"]);
      n += Rcpp::as<int>(get_n_activities());
    }
  }
  
  ~Branch() {
    merge.clear();
    path.clear();
    pending.clear();
  }
  
  void show(int indent=0) {
    for (unsigned int i = 0; i < trj.size(); i++) {
      Activity::show(indent);
      Rcpp::Rcout << "merge: " << merge[i] << " }" << std::endl;
      Rcpp::Function show(trj[i]["show"]);
      show(indent+2);
    }
  }
  
  double run(Arrival* arrival) {
    std::set<Arrival*>::iterator search = pending.find(arrival);
    if (search != pending.end())
      pending.erase(arrival);
    else {
      unsigned int i = Rcpp::as<unsigned int>(option());
      if (i < 0 || i >= path.size())
        Rcpp::stop("index out of range");
      selected = path[i];
      if (merge[i])
        pending.insert(arrival);
    }
    return 0;
  }
  
  Activity* get_next() {
    if (selected) {
      Activity* aux = selected;
      selected = NULL;
      return aux;
    } else 
      return Activity::get_next();
  }
  
private:
  Rcpp::Function option;
  std::vector<bool> merge;
  std::vector<Rcpp::Environment> trj;
  Activity* selected;
  std::vector<Activity*> path;
  std::set<Arrival*> pending;
};

#endif
