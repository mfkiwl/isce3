
#include "../core/Constants.h"
#include "IH5.h"



///////////////////////// UTILITIES ///////////////////////////////////





void attrsNames(H5::H5Object &loc, H5std_string nameAttr, void *opdata){
   auto up = reinterpret_cast<std::vector<std::string> *>(opdata);
   up->push_back(nameAttr);
}



// User function to be passed to HDF5 *visit* function that recursively return 
// all objects contained in the file (or from a given path) and to which is 
// applied the user function.
// Implementation note: The function signature allows one user parameter (opdata)
// to be provided. We need 3 parameters: the vector containing the found objects, 
// the searched string, whether the user wants only DATASET, GROUP or BOTH, and if
// the user wants the search to occur from a given start in the tree.
// To keep things simple, the search string and object needed are stored in the 
// vector in the first and second location respectively. They are removed from 
// the vector once the search is completed  
herr_t matchName(hid_t loc_id, const char *name, const H5O_info_t *info, 
                     void *opdata) {
   
   //auto outList = reinterpret_cast<std::vector<std::string> *>(opdata);
   //auto outList = static_cast<std::vector<std::string> *>(opdata);
   auto meta = static_cast<isce::io::findMeta *>(opdata);

   // Check if the current object has a name containing the searched string.
   // If it does, check that its type (group/dataset) matches the user need.
   // Keep name of object (i.e., path+name) if success
   
   //std::regex pattern((*outList)[0]);
   std::regex pattern(meta->searchStr);
   if (std::regex_search(name, pattern)) {
      if(info->type == H5O_TYPE_GROUP && (meta->searchType).compare("GROUP") == 0)
         meta->outList.push_back((meta->basePath)+name);
      else if (info->type == H5O_TYPE_DATASET && (meta->searchType).compare("DATASET") == 0)
         meta->outList.push_back((meta->basePath)+name);
      else if (meta->searchType.compare("BOTH") == 0)
         meta->outList.push_back((meta->basePath)+name);
      else {}
   }

   return 0;

}





// This function is a search function allowing the user to list (in a 
// vector<string> container) all the group and dataset whose name 
// (i.e. path/name) contains a given string.
// IN: nameIn: String to be searched in the file tree
// IN start (default: "/"): Location at which the search is to be started. That is
// the function will search that location and all subdirectories.
// IN type (default: "BOTH"): Whether to return only DATASET, GROUP or BOTH
// Note that whether or not the user pass in a start path for the search, the 
// returned paths of the objects founds are always the full absolute path.

std::vector<std::string> findByName(hid_t loc_id,
                                    std::string nameIn, 
                                    std::string start,
                                    std::string type,
                                    std::string returnedPath) {

    isce::io::findMeta findMetaInfo;

    if (nameIn.empty())
        return findMetaInfo.outList;

/*
    if (start != "/" && start != ".") {
        // Check that the provided start path exists in the file
        status = H5Lexists(loc_id, start.c_str(), lapl_id);
        if (status <= 0)
            return findMetaInfo.outList;
        // Check that the provided start path points to a group
        herr_t status = H5Oget_info_by_name(loc_id, start.c_str(), 
                                             &info, H5P_DEFAULT);
        if (status < 0 || info.type != H5O_TYPE_GROUP)
           return findMetaInfo.outList;
    }
*/    
    if (type != "GROUP" && type != "DATASET" && type != "BOTH")
        return findMetaInfo.outList;

 
    findMetaInfo.searchStr = nameIn;
    findMetaInfo.searchType = type;
    if (returnedPath == "FULL"){
       findMetaInfo.basePath.append(start);
       if (start == ".")
           findMetaInfo.basePath.erase(0,1);
       if (start.length() > 1 && strncmp(&start.back(),"/",1))
          findMetaInfo.basePath.append("/");
    }


    H5Ovisit_by_name(loc_id, start.c_str(), H5_INDEX_NAME, H5_ITER_INC, 
                     matchName, &findMetaInfo, H5P_DEFAULT);


    return findMetaInfo.outList; 

}




///////////////////////// DATASET ///////////////////////////////////




H5::DataSpace isce::io::IDataSet::getDataSpace(const std::string &v) {

    H5::DataSpace dspace;

    // Looking for the dataset itself
    if (v.empty()) 

        // Open the dataspace of the current dataset
        dspace = H5::DataSet::getSpace();

    // Looking for the attribute of name v contained in the dataset itself
    else if (attrExists(v)){    

       // Open the attribute of the given name
       H5::Attribute attr = openAttribute(v);

       // Open the dataspace of the current attribute
       dspace = attr.getSpace();
        
       // Close the attribute
       attr.close();
    }
    else {}//TODO throw exception

    return dspace;
}


/** @param[in] v Name of the attribute (optional).
 *  Returns the number of dimension of the dataset or attribute.
 *
 *  If input is not empty, returns the number of dimension of the given 
 *  attribute name. If input is empty, returns the number of dimension of 
 *  current dataset.
 *  Scalar:0; 1D array:1, 2D array:2, etc. */
int isce::io::IDataSet::getRank(const std::string &v) {

    // Get the dataspace of dataset or attribute
    H5::DataSpace dspace = getDataSpace(v);

    // Get the rank of the data
    int rank = dspace.getSimpleExtentNdims();

    // Close the dataspace
    dspace.close();

    return rank;    
};


/** @param[in] v Name of the attribute (optional).
 *  Returns the number of elements in the dataset or attribute.
 *
 *  If input is not empty, returns the number of elements in the given 
 *  attribute. If input is empty, returns the number of elements in the current
 *  dataset. */
int isce::io::IDataSet::getNumElements(const std::string &v) {

    // Get the dataspace of dataset or attribute
    H5::DataSpace dspace = getDataSpace(v);
  
    // Get the total number of elements in the dataset
    int nbElements = dspace.getSimpleExtentNpoints();

    // Close the dataspace
    dspace.close();

    return nbElements;
}



/** @param[in] v Name of the attribute (optional).
 *  Returns a std::vector containing the number of elements in each dimension of
 *  the dataset or attribute.
 *
 *  If input is not empty, returns the number of elements in each dimension of
 *  the given attribute. If input is empty, returns the number of elements in 
 *  each dimension of the current dataset. 
 *  A scalar dataset/attribute will return an empty std::vector. */
std::vector<int> isce::io::IDataSet::getDimensions(const std::string &v) {

    // Get the dataspace of dataset or attribute
    H5::DataSpace dspace = getDataSpace(v);

    // Get the rank of the dataset, i.e., the number of 
    // dimension
    int rank = dspace.getSimpleExtentNdims();

    // Initialize the vector that will be returned to the
    // caller
    std::vector<int> outDims;

    // If dataset is not a scalar proceed
    if (rank) {
	   // Get dimensions    
       hsize_t dims[rank];
       dspace.getSimpleExtentDims(dims, NULL);

       // Fill the output vector
       for (int i=0; i<rank; i++)
          outDims.push_back(dims[i]);
    }

    // Close the dataspace
    dspace.close();

    return outDims;
};


/** @param[in] v Name of the attribute (optional).
 *  Returns the  HDF5 class type of the data contained in the dataset or 
 *  attribute.
 *
 *  If input is not empty, returns the class type of the given attribute. If 
 *  input is empty, returns the class type.
 *  This information is useful to provide the HDF% Read function with a container 
 *  of the proper type. The HFD5 library will do type conversion when possible, 
 *  but will throw an error if types are incompatible. */
std::string isce::io::IDataSet::getTypeClassStr(const std::string &v) {

    H5T_class_t type;

    // Looking for the dataset data type
    if (v.empty())
        // Get the data type class of the current dataset
        type = H5::DataSet::getTypeClass();  
    // Looking for the attribute v data type
    else if (attrExists(v)){
        // Open the attribute of the given name
        H5::Attribute attr = H5::DataSet::openAttribute(v);
    
        // Get the Class type of the attribute type
        type = attr.getTypeClass();  

        // Close the open attribute
        attr.close();
    }
    else {
       return "Unknown data type";
       //TODO Should throw an error
    }

    // Return a human-readable attribute type
    switch (type) {
        case H5T_STRING:
            return "H5T_STRING";
            break;
        case H5T_INTEGER:
            return "H5T_INTEGER";
            break;
        case H5T_FLOAT:
            return "H5T_FLOAT";
            break;
        case H5T_TIME:
            return "H5T_TIME";
            break;
        case H5T_BITFIELD:
            return "H5T_BITFIELD";
            break;
        case H5T_OPAQUE:
            return "H5T_OPAQUE";
            break;
        case H5T_COMPOUND:
            return "H5T_COMPOUND";
            break;
        case H5T_REFERENCE:
            return "H5T_REFERENCE";
            break;
        case H5T_ENUM:
            return "H5T_ENUM";
            break;
        case H5T_VLEN:
            return "H5T_VLEN";
            break;
        case H5T_ARRAY:
            return "H5T_ARRAY";
            break;
        default:
            return "Unknown data type";
            break;
    }
}




/** Return the names of all the attributes attached to the current dataset. */
std::vector<std::string> isce::io::IDataSet::getAttrs() {

    // Initialize container that will contain the attribute names (if any)
    std::vector<std::string> outList;

    // If none, job done, return
    if (H5::H5Object::getNumAttrs() == 0)
        return outList;

    // Iterate over all the attributes and get their names
    H5::H5Object::iterateAttrs(attrsNames , NULL, &outList);
    //int idx = H5::H5Object::iterateAttrs(attrsNames , NULL, &outList);

    return outList;
}



/** Return the chunk size of the dataset storage layout.
 * 
 * The size of the returned vector is the same as the rank of the dataset and 
 * contains the size of the chunk in each dimensions. A size of 0 means no 
 * chunking in that dimension. */ 

//(TODO To be confirmed)

std::vector<int> isce::io::IDataSet::getChunkSize() {

    // First, get the rank of the dataset
    int rank = getRank();

    // Initialize the output container
    std::vector<int> out(rank,0);

    // Get the dataset creation properties list
    H5::DSetCreatPropList plist = getCreatePlist();

    // Check that the dataset is chunked
    if (H5D_CHUNKED == plist.getLayout()) {
        
       // Initialize array to get chunk size
       hsize_t * chunkDims = new hsize_t[rank];   //TODO check for success

       // Get the chunk size
       // rankChunk contains the dimensionality of the chunk. If negative, 
       // then function failed.
       plist.getChunk(rank, chunkDims);
       //int rankChunk = plist.getChunk(rank, chunkDims);

       // TODO check for success. If failure, do ?
	
       // Fill in the output vector
       for (int i=0; i<rank; i++)
          out[i] = (int)chunkDims[i];

       // Close obj and release temporary array
       delete [] chunkDims;
       plist.close();

    }
        
    return out;

}


/** @param[in] v Name of the attribute (optional).
 *  Returns the actual number of bit used to store the current dataset or given 
 *  attribute data in the file. */
int isce::io::IDataSet::getNumBits(const std::string &v) {

    H5::DataType dtype;

    // Looking for the dataset itself
    if (v.empty()) 
       // Open the dataspace of the current dataset
       dtype = H5::DataSet::getDataType();

    // Looking for the attribute of name v contained in the dataset itself
    else if (attrExists(v)){    
        // Open the attribute of the given name
        H5::Attribute attr = openAttribute(v);

        // Open the dataspace of the current attribute
        dtype = attr.getDataType();
        
       // Close the attribute
       attr.close();
    }
    else {}//TODO throw exception

    // Note: from there, the C interface is used as it seems that the C++ 
    // interface does not have all the functionalities implemented
    
    // Get the id (for C interface) of the dataType object
    hid_t dt = dtype.getId();

    // Get the precision in bit of the datatype. 
    size_t precision = H5Tget_precision(dt);

    dtype.close();
    H5Tclose(dt);

    return (int) precision;
};

/** @param[out] str String representation for GDAL's IH5Dataset driver

  Returns IH5:::ID=string*/
std::string isce::io::IDataSet::toGDAL()
{
    std::string result = "IH5:::ID=" + std::to_string(getId());
    return result;
};

/** @param[in] att  Name of the attribute (optional).
 *  @param[out] v Dataset or attribute string value.
 *
 *  If input is not empty, reads the attributes string value, otherwise, 
 *  reads the dataset scalar string value.
 *  For dataset/attributes containing more than one elements, see other
 *  function signature. */
void isce::io::IDataSet::read(std::string &v, const std::string &att) {

    // Check that the dataset/attribute contains a scalar, i.e., rank=0
    if (getRank(att) != 0)
        return;   // TODO Should add a message or something here

    if (att.empty()) {
       H5::DataSet::read(v, getStrType());
    }
    else if (attrExists(att)) {
       // Open the attribute
       H5::Attribute a = openAttribute(att);

       // Read the attribute from file
       a.read(a.getStrType(), v);  // Note: order of parameter is reversed 
                                    //       compared to dataset read
       // Close the attribute
       a.close();
    }
    else {}   // TODO Should add a message or something here

    return;
}


/** @param[in] att String containing the name of the attribute to read.
 *  @param[out] buffer Raw pointer to array that will received the read data.
 *
 *  buffer has to be adequately allocated by caller. */
void isce::io::IDataSet::read(std::string* buffer, const std::string &att) { 

    // Check that attribute name is not null or empty
    if (!attrExists(att)) {
        std::cout << "Warning: Attribute: " << att << " does not exist!" << std::endl;
        return;
    }

    // Open the attribute
    H5::Attribute a = openAttribute(att);

    // Get the number of elements in the attribute
    int nbElements = getNumElements(att);

    // Read the attribute string array as char* per H5 library requirements
    char *rdata = new char[nbElements*isce::io::STRLENGTH];
    char* p= &rdata[0];
    try {
       a.read(getH5Type<std::string>(), (void*)rdata);
       //Assign raw buffer data to strings
       for(int iStr=0; iStr<nbElements; iStr++) {
          buffer[iStr] = std::string(p);
          p += isce::io::STRLENGTH;
       }
       delete[] rdata;
    }
    catch(...) {
       delete[] rdata;
       std::cout << "ERROR while reading std::string dataset" << std::endl;
       return;
     }


    // Close the attribute
    a.close();
}



/** @param[in] dspace DataSpace of the subset to read
 *  @param[out] buffer Raw pointer to std::string array that will received the read data.
 *
 *  buffer has to be adequately allocated by caller. */
void isce::io::IDataSet::read(std::string * buffer, const H5::DataSpace& dspace) { 
 
    // Check that the selection is valid (no out of bound)
    // TODO - to be adjusted with preferred error/message handling
    if (!dspace.selectValid()){
       std::cout << "Sub-selection of dataset is invalid" << std::endl;
       return;
    }

    // Get total number of elements to read
    hssize_t nbElements = dspace.getSelectNpoints();
    
    // Format the dataspace of the memory to receive the data read from file
    H5::DataSpace memspace = getMemorySpace((hsize_t)nbElements);

    // Read the dataset to memory
    // This is where specifics to std::string are handled
    // H5::DataSet::read(buffer, getH5Type<T>() , memspace, dspace);

    // Make sure we're dealing with strings
    if (getDataType().getClass() != H5T_STRING) {
       std::cout << "ERROR: Not a String class dataset!" << std::endl;
       return;
    }

    char *rdata = new char[nbElements*isce::io::STRLENGTH];
    char* p= &rdata[0];
    try {
       H5::DataSet::read((void*)rdata, getDataType() , memspace, dspace);
       //Assign raw buffer data to strings
       for(int iStr=0; iStr<nbElements; iStr++) {
          buffer[iStr] = std::string(p);
          p += isce::io::STRLENGTH;
       }
       delete[] rdata;
    }
    catch(...) {
       //for(int iStr=0; iStr<nbElements; ++iStr)
       //    delete[] rdata[iStr];
       delete[] rdata;
       std::cout << "ERROR while reading std::string dataset" << std::endl;
       return;
     }

    // Close memory dataspaces
    memspace.close();

}






/** @param[in] pointer to std::string buffer to write to dataset.
 *  @param[in] DataSpace object for locating where to write the data in the dataset.
 *  */
void isce::io::IDataSet::write(const std::string* buf, const H5::DataSpace& dspace) {


   // Check that the selection is valid (no out of bound)
   // TODO - to be adjusted with preferred error/message handling
   if (!dspace.selectValid()){
      std::cout << "Sub-selection of dataset is invalid" << std::endl;
      return;
   }

   // Construct the memory dataspace (i.e., dataspace of buf). For now, and that
   // may change in a the future, there is no possibility to subset buf. The 
   // full content of buf is to be selected. Buf dataspace is set to a 1D array 
   // of size equals to the number of elements in the vector.
   H5::DataSpace mspace = getMemorySpace(dspace.getSelectNpoints()); 


   // Get the datatype of the dataset
   H5::DataType dtype =  getDataType();

    // Make sure we're dealing with strings
    if (dtype.getClass() != H5T_STRING) {
       std::cout << "ERROR: Not a String class dataset!" << std::endl;
       return;
    }

   //Convert the array of string into a C string array.
   //because the input function ::write requires that.
   char *cStrArray = new char[dspace.getSelectNpoints() * isce::io::STRLENGTH];
   for(int i=0; i < dspace.getSelectNpoints(); i++) {
       std::strncpy(cStrArray, buf[i].c_str(), isce::io::STRLENGTH);
       cStrArray[isce::io::STRLENGTH-1]= '\0';
       cStrArray += isce::io::STRLENGTH;
   }
   cStrArray -= dspace.getSelectNpoints() * isce::io::STRLENGTH;

   //Write data to dataset
   H5::DataSet::write(cStrArray, dtype);

   delete [] cStrArray;

   dtype.close();
   mspace.close();
}






/** 
 * @param[in] name Name of the dataset attribute to create
 * @param[in] datatype H5 data type of the attribute
 * @param[in] dataspace H5 data space of the attribute
 * @param[in] buffer of std::string values to write
 */
void isce::io::IDataSet::createAttribute(
        const std::string& name, 
        const H5::DataType& datatype,
        const H5::DataSpace& dataspace,
        const std::string * buffer) {

   if (name.empty()) {
       std::cout << "Can't have an empty name for Attribute creation!" << std::endl;
       //THROW ERROR
   }

   if (attrExists(name.c_str())) {
       std::cout << "Attribute already exists!" << std::endl;
       //THROW ERROR
   }

   // Create the attribute
   H5::Attribute attribute =  H5::H5Object::createAttribute(name, datatype, dataspace);

   // Write the attribute data. 
   // Specific process in case of std::string
   if (datatype.getClass() == H5T_STRING) {
   
      //Convert the array of string into a C string array.
      //because the input function ::write requires that.
      char *cStrArray = new char[dataspace.getSelectNpoints() * isce::io::STRLENGTH];
      for(int i=0; i < dataspace.getSelectNpoints(); i++) {
         std::strncpy(cStrArray, buffer[i].c_str(), isce::io::STRLENGTH);
         cStrArray[isce::io::STRLENGTH-1]= '\0';
         cStrArray += isce::io::STRLENGTH;
       }
       cStrArray -= dataspace.getSelectNpoints() * isce::io::STRLENGTH;
       attribute.write(datatype, cStrArray);
       delete [] cStrArray;
   }
   else 
       std::cout << "Error: Attribute type is not of string type!" << std::endl;

   attribute.close();

}





H5::DataSpace isce::io::IDataSet::getDataSpace(const int * startIn, 
                                                   const int * countIn, 
                                                   const int * strideIn) { 
 
    // Get information of the file dataspace       
    H5::DataSpace dspace = H5::DataSet::getSpace();	 
    int rank = dspace.getSimpleExtentNdims();
  
    // Specific case if dataset contains a scalar
    if (rank == 0)
        return dspace;


    // HDF5 library expect startIn/countIn/strideIn to be in unsigned long long.
    // Convert input (int) and set default values
    // It is assumed that startIn, countIn, strideIn length are equal
    // to rank of the dataset
    
    // Get the start locations for each dimension of the dataset
    hsize_t *start = new hsize_t[rank]();
    if (startIn) {
        for(int i=0; i<rank; i++)
             start[i] = (hsize_t)startIn[i];
    }

    // Get the number of elements to read in each dimension
    hsize_t *count = new hsize_t[rank];
    if (countIn) {
        for (auto i=0; i<rank; i++)
            count[i] = (hsize_t)countIn[i];
    }
    else
        dspace.getSimpleExtentDims(count, NULL);


    // Get the stride of the reading in each dimension
    hsize_t *stride = new hsize_t[rank];
    std::fill_n(stride, rank, 1);
    if (strideIn) {
        for (int i=0; i<rank; i++)
             stride[i] = (hsize_t)strideIn[i];
    }



    // Select the subset of the dataset to read
    // Note: H5 throws an error if this function is applied on a scalar dataset
    dspace.selectHyperslab(H5S_SELECT_SET, count, start, stride);

    delete [] start;
    delete [] count;
    delete [] stride;

    return dspace;
}




H5::DataSpace isce::io::IDataSet::getDataSpace(const std::vector<std::slice> * slicesIn) { 
 
    // Get information of the file dataspace       
    H5::DataSpace dspace = H5::DataSet::getSpace();	 
    int rank = dspace.getSimpleExtentNdims();

    // Specific case if dataset contains a scalar
    if (rank == 0)
        return dspace;


    // Get the start locations for each dimension of the dataset
    // and initialize them to origin (i.e., 0)
    hsize_t *start = new hsize_t[rank]();

    // Get the number of elements to read in each dimension
    // and initialize them to full dimension
    hsize_t *count = new hsize_t[rank];
    dspace.getSimpleExtentDims(count, NULL);

    // Get the stride of the reading in each dimension
    // and initialize them to a stride of 1
    hsize_t *stride = new hsize_t[rank];
    std::fill_n(stride, rank, 1);

    
    // Now modify the start/count/stride with values from the std::slice
    // Note: If sliceIn contains more slices than the rank of the dataset, only
    // the N=rank first slices will be used. If sliceIn contains less slices 
    // than the rank of the dataset, the slices will be applied to the first
    // dimensions and the rest of the dimensions will be set to full extent 
    // (cf. initialization above).  
    for (int i=0; i<slicesIn->size() && i<rank; i++) {
        start[i] = (hsize_t)(*slicesIn)[i].start();
        count[i] = (hsize_t)(*slicesIn)[i].size();
        stride[i] = (hsize_t)(*slicesIn)[i].stride();
    }

    // Select the subset of the dataset to read
    // Note: H5 throws an error if this function is applied on a scalar dataset
    dspace.selectHyperslab(H5S_SELECT_SET, count, start, stride);

    delete [] start;
    delete [] count;
    delete [] stride;

    return dspace;
}





H5::DataSpace isce::io::IDataSet::getDataSpace(const std::gslice * gsliceIn) { 
 
    // Get information of the file dataspace       
    H5::DataSpace dspace = H5::DataSet::getSpace();	 
    int rank = dspace.getSimpleExtentNdims();

    // Specific case if dataset contains a scalar
    if (rank == 0)
        return dspace;

    
    // Get the start locations for each dimension of the dataset
    // and initialize them to origin (i.e., 0)
    hsize_t *start = new hsize_t[rank]();

    // Get the number of elements to read in each dimension
    // and initialize them to full dimension
    hsize_t *count = new hsize_t[rank];
    dspace.getSimpleExtentDims(count, NULL);

    // Get the stride of the reading in each dimension
    // and initialize them to a stride of 1
    hsize_t *stride = new hsize_t[rank];
    std::fill_n(stride, rank, 1);

    
    // Modify the start/count/stride arrays with values from the std::gslice
    // Note: If gslice contains more *dimensions* than the rank of the dataset, 
    // only the N=rank first inner dimensions will be used. If gsliceIn contains
    // less *dimensions* than the rank of the dataset, the slices will be 
    // applied to the first outer dimensions and the rest of the dimensions will
    // be set to full extent 

    // First, convert gslice.start() which is a scalar indicating the location
    // of the first pixel to read in the entire dataset into an array of
    // locations indicating the location in each dimension
    long dims = dspace.getSimpleExtentNpoints();
    long tot = gsliceIn->start();
    for (int i=0; i<rank; i++) {
       dims = dims / count[rank-1-i];
       //start[rank-1-i] = tot / dims;
       start[i] = tot / dims;
       tot = tot % dims;
    }

    // Loading the size and stride of each dimension
    // TODO: Should check/warn if gslice size and stride number of elements are
    // are not identical to rank of dataset; or if number of elements between 
    // size and stride are different
    // Converting gslice stride to HDF5 strides. gslice stride are absolute, 
    // whereas HDF5 strides are expressed for a given dimension
    long strideStep = 1;
    hsize_t gsliceDims = ((*gsliceIn).stride()).size();
    for (int i=0; i<gsliceDims && i<rank; i++) {
        stride[i] = (hsize_t)((*gsliceIn).stride())[gsliceDims-1-i] / strideStep;
        strideStep *= count[i];
    }

    // Converting gslice size to HDF5 count. It's essentially the same thing
    for (int i=0; i<gsliceDims && i<rank; i++) {
        count[i] = (hsize_t)((*gsliceIn).size())[i];
    }

    // Select the subset of the dataset to read
    // Note: H5 throws an error if this function is applied on a scalar dataset
    dspace.selectHyperslab(H5S_SELECT_SET, count, start, stride);

    delete [] start;
    delete [] count;
    delete [] stride;

    return dspace;
}




H5::DataSpace isce::io::IDataSet::getDataSpace(const size_t xidx, const size_t yidx, const size_t iowidth, const size_t iolength, const size_t band) { 

    int index = 0;

    // Get information of the file dataspace       
    H5::DataSpace dspace = H5::DataSet::getSpace();	 
    int rank = dspace.getSimpleExtentNdims();

    // Specific case if dataset contains a scalar
    if (rank == 0)
        return dspace;
    if (rank > 3) {
       std::cout << "Error: the use of get/setBlock methods on Hdf5 dataset are only valid for dataset with rank <= 3" << std::endl;
       return dspace;
    }
    

    // Get the start locations for each dimension of the dataset
    // and initialize them to origin (i.e., 0)
    hsize_t *start = new hsize_t[rank]();
    if (rank == 3) start[index++] = static_cast<hsize_t>(band);
    start[index++] = static_cast<hsize_t>(yidx); 
    start[index] = static_cast<hsize_t>(xidx); 

    // Get the number of elements to read in each dimension
    // and initialize them to full dimension
    index = 0;
    hsize_t *count = new hsize_t[rank];
    if (rank == 3) count[index++] = static_cast<hsize_t>(1);
    count[index++] = static_cast<hsize_t>(iolength); 
    count[index] = static_cast<hsize_t>(iowidth); 

    // Get the stride of the reading in each dimension
    // and initialize them to a stride of 1
    hsize_t *stride = new hsize_t[rank];
    std::fill_n(stride, rank, 1);

    // Select the subset of the dataset to read
    // Note: H5 throws an error if this function is applied on a scalar dataset
    dspace.selectHyperslab(H5S_SELECT_SET, count, start, stride);

    delete [] start;
    delete [] count;
    delete [] stride;

    return dspace;
}








///////////////////////// GROUP ///////////////////////////////////


std::vector<std::string> isce::io::IGroup::getAttrs() {

    // Initialize container that will contain the attribute names (if any) and 
    // returned to the caller
    std::vector<std::string> outList;

    // If none, job done, return
    if (H5::H5Object::getNumAttrs() == 0)
        return outList;

    // Iterate over all the attributes and get their names
    H5::H5Object::iterateAttrs(attrsNames , NULL, &outList);

    return outList;
}


/** @param[in] name Regular Expression to search for.
 *  @param[in] start Relative path from current group to start the search from. 
 *  @param[in] type Type of object to search for. Default: BOTH
 *  @param[in] returnedPath Absolute or Relative path of found object. Default: FULL
 *
 *  The function returns paths of all objects in the file whose names satisfy name. 
 *
 *  param name: can be a regular expression.
 *  param type: three types of objects to search for are available:
 *     GROUP: only returns groups whose names satisfy the inpout name.
 *     DATASET: only return datasets whose names satisfy the input name. 
 *     BOTH: return groups and datsets whose names satisfy the input name.
 *  param returnedPath: the returned path of the found objects can be expressed 
 *  from the current group (returnedPath = FULL - this is default) or relative 
 *  to the start (returnedPath = RELATIVE).
 */
std::vector<std::string> isce::io::IGroup::find(const std::string name, 
                                                const std::string start, 
                                                const std::string type, 
                                                const std::string returnedPath){

   if (start.empty())
      return findByName(this->getId(), name, ".", type, returnedPath); 
   else
      return findByName(this->getId(), name, start, type, returnedPath); 

} 

/** Return the path of the current group from the root location in the file */
std::string isce::io::IGroup::getPathname()
{
    // Need first to get the length of the name
    size_t len = H5Iget_name(this->getId(), NULL, 0);

    // Set up a buffer correctly sized to receive the path of the group
    char buffer[len];

    // Get the path
    H5Iget_name(this->getId(),buffer,len+1);

    // Return as std::string
    std::string path = buffer;

    return path;
}




/** @param[in] v Name of the attribute.
 *  Returns the H5::DataSpace of the attribute.
 */
H5::DataSpace isce::io::IGroup::getDataSpace(const std::string &name) {

    H5::DataSpace dspace;

    // Looking for the attribute of name v contained in the dataset itself
    if (attrExists(name)){    

       // Open the attribute of the given name
       H5::Attribute attr = openAttribute(name);

       // Open the dataspace of the current attribute
       dspace = attr.getSpace();
        
       // Close the attribute
       attr.close();
    }
    else {
       std::cout << "Warning: Attribute: " << name << " does not exists" <<std::endl;
    }//TODO throw exception

    return dspace;
}




/** @param[in] v Name of the attribute.
 *  Returns the number of elements in the attribute.
 */
int isce::io::IGroup::getNumElements(const std::string &name) {

    int nbElements = -1;

    if (attrExists(name)){    
       // Get the dataspace of dataset or attribute
       H5::DataSpace dspace = getDataSpace(name);
  
       // Get the total number of elements in the dataset
       nbElements = dspace.getSimpleExtentNpoints();

       // Close the dataspace
       dspace.close();
    }
    else {
       std::cout << "Warning: Attribute: " << name << " does not exists" <<std::endl;
    }//TODO throw exception

    return nbElements;
}





/** Open the dataset of the input name that belongs to the current group */
isce::io::IDataSet isce::io::IGroup::openDataSet(const H5std_string &name) {

    H5::DataSet dset = H5::Group::openDataSet(name);
    return IDataSet(dset);

};


/** @param[in] name Name of the group to open.
 *
 * name must contain the full path from root location and name of the group
 * to open. */
isce::io::IGroup isce::io::IGroup::openGroup(const H5std_string &name) {
            
    H5::Group group = H5::Group::openGroup(name);
    return IGroup(group);

};


/** @param[in] att  Name of the attribute 
 *  @param[out] v   Attribute scalar value (std::string type).
 *
 *  Reads the value of a string attribute contained in the current group.
 *  For attribute containing more than one elements, see other function 
 *  signature. */
void isce::io::IGroup::read(std::string &v, const std::string &att) {

    if (attrExists(att)) {
       // Open the attribute
       H5::Attribute a = openAttribute(att);

       // Get the dataSpace of the attribute
       H5::DataSpace dspace = a.getSpace();

       // If attribute contains a scalar, as it should, read the attribute's 
       // value
       if (dspace.getSimpleExtentNdims() == 0)
          //a.read(memType(v), v);
          //a.read(getH5Type<decltype(v)>(), v);
          a.read(getH5Type<std::string>(), v);
       
       else {} // TODO warning/trow? 
                                   
                                            
       // Close the attribute and dataspace
       a.close();
       dspace.close();
    }
    else {}   // TODO Should warn or throw here

    return;
}



/** @param[in] att String containing the name of the attribute to read.
 *  @param[out] buffer Raw pointer to array that will received the read data.
 *
 *  buffer has to be adequately allocated by caller. */
void isce::io::IGroup::read(std::string* buffer, const std::string &att) { 

    // Check that attribute name is not null or empty
    if (!attrExists(att)) {
        std::cout << "Warning: Attribute: " << att << " does not exist!" << std::endl;
        return;
    }

    // Open the attribute
    H5::Attribute a = openAttribute(att);

    // Get the number of elements in the attribute
    int nbElements = getNumElements(att);

    // Read the attribute string array as char* per H5 library requirements
    char *rdata = new char[nbElements*isce::io::STRLENGTH];
    char* p= &rdata[0];
    try {
       a.read(getH5Type<std::string>(), (void*)rdata);
       //Assign raw buffer data to strings
       for(int iStr=0; iStr<nbElements; iStr++) {
          buffer[iStr] = std::string(p);
          p += isce::io::STRLENGTH;
       }
       delete[] rdata;
    }
    catch(...) {
       delete[] rdata;
       std::cout << "ERROR while reading std::string dataset" << std::endl;
       return;
     }


    // Close the attribute
    a.close();
}



/** @param[in] name Name of the group to create.
 *
 * name must contain the full path from root location and name of the group
 * to create. */
isce::io::IGroup isce::io::IGroup::createGroup(const H5std_string &name) {
            
    // First activate the automatic creation of intermediate groups if they
    // don't exist. Needs the C interface.
    hid_t gcpl = H5Pcreate(H5P_LINK_CREATE);
    herr_t status = H5Pset_create_intermediate_group(gcpl, 1);

    hid_t group = H5Gcreate(this->getId(), name.c_str(), gcpl, H5P_DEFAULT, H5P_DEFAULT);
    H5Pclose(gcpl);

    return IGroup(group);
};




/** 
 * @param[in] name Name of the dataset to create
 * @param[in] data Scalar (std::string) data to write to the dataset
 *
 * The dataset is a 1-element dataset
 */
isce::io::IDataSet isce::io::IGroup::createDataSet(
                const std::string& name,
                const std::string& data) {

   //return createDataSet(name, &data, 1);

   H5::DataType dtype = getH5Type<std::string>();
   H5::DataSet dset =  H5::Group::createDataSet(name, dtype, H5::DataSpace());
   dset.write(data, dtype);

   dtype.close();

   return IDataSet(dset);

}



/** 
 * @param[in] name Name of the group attribute to create
 * @param[in] datatype H5 data type of the attribute
 * @param[in] dataspace H5 data space of the attribute
 * @param[in] buffer of std::string values to write
 */
void isce::io::IGroup::createAttribute(
        const std::string& name, 
        const H5::DataType& datatype,
        const H5::DataSpace& dataspace,
        const std::string * buffer) {

   if (name.empty()) {
       std::cout << "Can't have an empty name for Attribute creation!" << std::endl;
       //THROW ERROR
   }

   if (attrExists(name.c_str())) {
       std::cout << "Attribute already exists!" << std::endl;
       //THROW ERROR
   }

   // Create the attribute
   H5::Attribute attribute =  H5::H5Object::createAttribute(name, datatype, dataspace);

   // Write the attribute data. 
   // Specific process in case of std::string
   if (datatype.getClass() == H5T_STRING) {
   
      //Convert the array of string into a C string array.
      //because the input function ::write requires that.
      char *cStrArray = new char[dataspace.getSelectNpoints() * isce::io::STRLENGTH];
      for(int i=0; i < dataspace.getSelectNpoints(); i++) {
         std::strncpy(cStrArray, buffer[i].c_str(), isce::io::STRLENGTH);
         cStrArray[isce::io::STRLENGTH-1]= '\0';
         cStrArray += isce::io::STRLENGTH;
       }
       cStrArray -= dataspace.getSelectNpoints() * isce::io::STRLENGTH;
       attribute.write(datatype, cStrArray);
       delete [] cStrArray;
   }
   else 
       std::cout << "Error: Attribute type is not of string type!" << std::endl;

   attribute.close();

}















///////////////////////// FILE ///////////////////////////////////





/** @param[in] name Name of the dataset to open.
 *
 * name must contain the full path from root location and name of the dataset
 * to open. */
isce::io::IDataSet isce::io::IH5File::openDataSet(const H5std_string &name) {

    H5::DataSet dset = H5::H5File::openDataSet(name);
    return IDataSet(dset);

};



/** @param[in] name Name of the group to open.
 *
 * name must contain the full path from root location and name of the group
 * to open. */
isce::io::IGroup isce::io::IH5File::openGroup(const H5std_string &name) {
            
    H5::Group group = H5::H5File::openGroup(name);
    return IGroup(group);

};


/** @param[in] name Name of the group to create.
 *
 * name must contain the full path from root location and name of the group
 * to create. */
isce::io::IGroup isce::io::IH5File::createGroup(const H5std_string &name) {
            
    // First activate the automatic creation of intermediate groups if they
    // don't exist. Needs the C interface.
    hid_t gcpl = H5Pcreate(H5P_LINK_CREATE);
    herr_t status = H5Pset_create_intermediate_group(gcpl, 1);

    hid_t group = H5Gcreate(this->getId(), name.c_str(), gcpl, H5P_DEFAULT, H5P_DEFAULT);
    H5Pclose(gcpl);

    //H5::Group group2 = H5::Group(group);
    //return IGroup(group2);
    return IGroup(group);
};


/** @param[in] name Regular Expression to search for.
 *  @param[in] start Relative path from root to start the search from. Default:file root
 *  @param[in] type Type of object to search for. Default: BOTH
 *  @param[in] returnedPath Absolute or Relative path of found object. Default: FULL
 *
 *  The function returns paths of all objects in the file whose names satisfy the 
 *
 *  param name: can be a regular expression.
 *  param type: three types of objects to search for are available:
 *     GROUP: only returns groups whose names satisfy the inpout name.
 *     DATASET: only return datasets whose names satisfy the input name. 
 *     BOTH: return groups and datsets whose names satisfy the input name.
 *  param returnedPath: the returned path of the found objects can be expressed 
 *  from the root (returnedPath = FULL - this is default) or relative to the 
 *  start (returnedPath = RELATIVE).
 */
std::vector<std::string> isce::io::IH5File::find(const std::string name, 
                                                 const std::string start, 
                                                 const std::string type, 
                                                 const std::string returnedPath){

   if (start.empty())
      return findByName(this->getId(), name, "/", type, returnedPath); 
   else
      return findByName(this->getId(), name, start, type, returnedPath); 

} 
