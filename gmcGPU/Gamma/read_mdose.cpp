
#include "read_mdose.h"




//  //read mdose file
GRID_POINTS read_mdose( string &filename){ 
  GRID_POINTS gp;

  //get the mdose file ready
  ifstream input_file(filename.c_str()); 
  if (!input_file) {
    std::cerr << "\nFailed to open " << filename<<endl;
    return gp; }

  string buff;
  getline(input_file,buff);
  getline(input_file,buff);
  getline(input_file,buff);
  getline(input_file,buff);
  getline(input_file,buff);
  getline(input_file,buff);
  getline(input_file,buff);
  getline(input_file,buff);
  getline(input_file,buff);
  gp.xg = parse_csv(buff);
  getline(input_file,buff);
  getline(input_file,buff);
  gp.yg = parse_csv(buff);
  getline(input_file,buff);
  getline(input_file,buff);
  gp.zg = parse_csv(buff);
  while(getline(input_file,buff)) {
    std::vector<float> line = parse_csv(buff);
    gp.D.insert(gp.D.end(), line.begin(), line.end());  //concatenate vectors
  }
  input_file.close();

  if (gp.D.size() != gp.xg.size()*gp.yg.size()*gp.zg.size()) {
    GRID_POINTS tmp;
    cout<<"ERROR: incorrect mdose format"<<endl;
    cout<<gp.xg.size()<<' '<<gp.yg.size()<<' '<<gp.zg.size()<<' '<<gp.D.size()<<endl;
    return tmp;
  }

  return gp;
}






std::vector<float> parse_csv(string &str) 
{
  std::vector<float> vect;
  std::stringstream ss(str);
  float i;
  while (ss >> i)
    {
      vect.push_back(i);
      if (ss.peek() == ',')
        ss.ignore();
    }
  return vect;
}

  
