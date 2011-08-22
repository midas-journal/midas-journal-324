/*=========================================================================
 * Automatic Junction Detection for Tubular Structures
 * Author:      Guanglei Xiong (guangleixiong at gmail.com)
 * Date:        March 23, 2009
 * Reference:   http://www.insight-journal.org/browse/publication/324
=========================================================================*/

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkJunctionDetectionFilter.h"

template<class TFilter>
class CommandProgressUpdate : public itk::Command 
{
public:
  typedef  CommandProgressUpdate    Self;
  typedef  itk::Command             Superclass;
  typedef  itk::SmartPointer<Self>  Pointer;
  itkNewMacro( Self );

  void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    Execute( (const itk::Object *) caller, event);
  }

  void Execute(const itk::Object * object, const itk::EventObject & event)
  {
    const TFilter * filter = dynamic_cast< const TFilter * >(object);
    if( !(itk::ProgressEvent().CheckEvent( &event )) )  return;
    std::cout << (int)(100 * filter->GetProgress()) << "% completed\r" << std::flush;
  }

protected:
  CommandProgressUpdate(){};
};

int main( int argc, char* argv[] )
{
  if( argc < 6 )
  {
    std::cerr << "Usage: " << std::endl;
    std::cerr << argv[0] << " inputImage outputImage outputjunctioninfoTXT inner outer" << std::endl;
    std::cerr << "Example: " << std::endl;
    std::cerr << argv[0] << " input.mha output.mha jcinfo.txt 2.0 3.0" << std::endl;
    return EXIT_FAILURE;
  }
  
  typedef itk::Image<unsigned short, 2>                       ImageType;
  typedef itk::ImageFileReader<ImageType>                     ReaderType;
  typedef itk::JunctionDetectionFilter<ImageType>             DetectorType;
  typedef itk::ImageFileWriter<DetectorType::OutputImageType> WriterType;

  char *inputfilename = argv[1];
  char *outputfilename = argv[2];
  char *jcinfofilename = argv[3];
  float inner = atof(argv[4]);
  float outer = atof(argv[5]);

  ImageType::Pointer inputImage;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( inputfilename );
  try 
  { 
    reader->Update();
    inputImage = reader->GetOutput();
  } 
  catch( itk::ExceptionObject & err ) 
  { 
    std::cerr << "ExceptionObject caught !" << std::endl; 
    std::cerr << err << std::endl; 
    return EXIT_FAILURE;
  }

  // Junction detection with a progress observer
  typedef CommandProgressUpdate<DetectorType> CommandType;
  CommandType::Pointer observer = CommandType::New();
  DetectorType::Pointer detector = DetectorType::New();
  detector->SetInnerRadius(inner);
  detector->SetOuterRadius(outer);
  detector->SetMinNumberOfPixel(6);
  detector->SetInput(reader->GetOutput());
  detector->AddObserver(itk::ProgressEvent(), observer);
  detector->Update();
  std::cout << std::endl;

  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( outputfilename );
  writer->SetInput( detector->GetOutput() );
  try 
  { 
    writer->Update(); 
  } 
  catch( itk::ExceptionObject & err ) 
  { 
    std::cerr << "ExceptionObject caught !" << std::endl; 
    std::cerr << err << std::endl; 
    return EXIT_FAILURE;
  }

  // Output junction infomation in TXT file
  std::ofstream jcinfo(jcinfofilename);
  if(!jcinfo.is_open())
  {
    std::cerr << "Cannot open " << jcinfofilename << " to write !" << std::endl;
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "jcLabel  jcIndex[0] jcIndex[1]  jcRadius" << std::endl;
    jcinfo << "jcLabel  jcIndex[0] jcIndex[1]  jcRadius" << std::endl;
  }
  const DetectorType::JCLabelMapType& jcLabelmap = detector->GetJCLabelMap();
  for( DetectorType::JCLabelMapType::const_iterator jclmIt = jcLabelmap.begin(); jclmIt!=jcLabelmap.end(); ++jclmIt )
  {
    int jcLabel = (*jclmIt).first;
    DetectorType::JCLabelPairType jclPair = (*jclmIt).second;
    ImageType::IndexType jcIndex = jclPair.first;
    ImageType::PointType point;
    inputImage->TransformIndexToPhysicalPoint(jcIndex, point);
    float jcRadius = jclPair.second;
    std::cout << jcLabel << "  " << jcIndex[0] << " " << jcIndex[1] << "  " << jcRadius << std::endl;
    jcinfo << jcLabel << "  " << jcIndex[0] << " " << jcIndex[1] << "  " << jcRadius << std::endl;
  }
  jcinfo.close();

  return EXIT_SUCCESS;
}
