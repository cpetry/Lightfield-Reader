

#include <Eigen/Dense>

#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "reconstruction3d.h"



reconstruction3D::reconstruction3D()
{
    //viewer = boost::shared_ptr<pcl::visualization::PCLVisualizer>(new pcl::visualization::PCLVisualizer("Simple Cloud Viewer"));
    cloud = pcl::PointCloud<pcl::PointXYZ>::Ptr(new pcl::PointCloud<pcl::PointXYZ>);


    //viewer->setShowFPS(true);
}

reconstruction3D::~reconstruction3D()
{

}
/*
void testPowerCrust(){
    QString file = QFileDialog::getOpenFileName( 0,
                               QString("Choose Depth map"),           // window name
                               "../",                                       // relative folder
                               QString("Depth Images(*.PNG *.JPG);;Point clouds (*.PCD)"));  // filetype
    if (!file.isNull()){
        vtkSmartPointer<vtkPLYReader> PlyReader = vtkSmartPointer<vtkPLYReader>::New();
        PlyReader->SetFileName ( file.toStdString().c_str() );
        PlyReader->Update();
        std::cout << "Number of points " << PlyReader->GetOutput()->GetPoints()->GetNumberOfPoints() << std::endl;

        vtkSmartPointer<vtkPowerCrustSurfaceReconstruction> SurfaceReconstructor = vtkSmartPointer<vtkPowerCrustSurfaceReconstruction>::New();
        SurfaceReconstructor->SetInput ( PlyReader->GetOutput() );
        SurfaceReconstructor->Update();

        std::string OutFileName = "Output";
        OutFileName.append ( ".ply" );
        std::cout << "Writing: " << OutFileName << std::endl;

        vtkSmartPointer<vtkPLYWriter> PlyWriter = vtkSmartPointer<vtkPLYWriter>::New();
        PlyWriter->SetFileName ( OutFileName.c_str() );
        PlyWriter->SetInput ( SurfaceReconstructor->GetOutput() );
        PlyWriter->Write();
    }
}*/

void reconstruction3D::calculatePointCloud(std::vector<cv::Mat> depthmaps){

    calculateArtificialPointCloud(depthmaps);
    //calculateRealPointCloud(depthmaps);
}

void reconstruction3D::calcPointCloudFromMultipleDepthMaps(){
    // QImage file
    QStringList files = QFileDialog::getOpenFileNames(NULL,
                               QString("Choose depth Images"), // window name
                               "../",                            // directory
                               QString("Images(*.PNG *.JPG)"));   // filetype

    if (!files.empty()){
        std::vector<cv::Mat> depthmaps;
        for (int i=0; i<files.length(); i++)
            depthmaps.push_back(cv::imread(files[i].toStdString(), cv::IMREAD_GRAYSCALE|cv::IMREAD_ANYDEPTH));
        calculatePointCloud(depthmaps);
    }

}


void reconstruction3D::calculateArtificialPointCloud(std::vector<cv::Mat> depthmaps){
    // set initial settings of point cloud
    cloud->width    = depthmaps[0].cols;
    cloud->height   = depthmaps[0].rows;
    cloud->is_dense = false;

    // calculate size of cloud
    int cloud_size = 0;
    for (int i=0; i<depthmaps.size(); i++){
        for(int y=0; y<cloud->height; y++){
            for(int x=0; x<cloud->width; x++){
                if (depthmaps[i].at<unsigned short>(y,x) > 0){
                    cloud_size++;
                }
            }
        }
    }

    // set size of cloud
    cloud->points.resize(cloud_size);



    // Fill in the cloud data
    // teapot width = 85mm // 195
    double radius = 190.0 * 1.81949; // 1
    double cam_height = 75.979 - 47.956; //147.956 - 47.956;
    double aspect_ratio = cloud->width * 1.0f / cloud->height;
    double camera_distance = std::sqrt(std::pow(radius,2) + std::pow(cam_height,2));
    double rot_cam = std::atan(cam_height / radius);
    double min_mm = 265.0; // 100.0f;
    double max_mm = 420.0; // 300.0f;
    double total_depth_size = max_mm - min_mm; //mm - maximum - minimum
    double fovy = 19.968 * M_PI / 180.0; // ;42.654
    double fovx = 28.452 * M_PI / 180.0; // 55.000
    double degree = 360.0 / 36.0;
    double width_mm = std::tan(fovx*0.5) * camera_distance * 2;
    double height_mm = std::tan(fovy*0.5) * camera_distance * 2;
    double height_half = cloud->height*0.5;
    double width_half = cloud->width*0.5;
    int depth_idx = 0;

    Eigen::Affine3f camrot_mat = Eigen::Affine3f::Identity();
    camrot_mat.rotate (Eigen::AngleAxisf (rot_cam, Eigen::Vector3f::UnitX()));

    // GAMMA was activated in 3DsMax :C!!!
    double bitdepth = std::pow(2, 16);

    for (int i=0; i<depthmaps.size(); i++){
        Eigen::Affine3f rot_mat = Eigen::Affine3f::Identity();
        rot_mat.rotate (Eigen::AngleAxisf (-degree*i * M_PI / 180.0, Eigen::Vector3f::UnitY()));

        for(int y=0; y<cloud->height; y++){
            for(int x=0; x<cloud->width; x++){
                if (depthmaps[i].at<unsigned short>(y,x) > 0){
                    pcl::PointXYZ& pt = cloud->points[depth_idx++];
                    unsigned short pixel_value = depthmaps[i].at<unsigned short>(y,x);
                    double d = pixel_value * 1.0 / bitdepth; // [0,1]
                    Eigen::Vector2d xy(std::abs(width_half - x)  / cloud->width * width_mm,
                                       std::abs(height_half - y) / cloud->height * height_mm);

                    double hypo = d*total_depth_size + min_mm;
                    double z = std::sqrt(std::abs(std::pow(hypo,2) - std::pow(xy.norm(),2))); // calculate z!!!

                    // a² = c² - b²
                    double depth = (z - min_mm);
                    Eigen::Vector3f v3((x * 1.0 / cloud->width  - 0.5),
                                        -(y * 1.0 / cloud->height - 0.5) / aspect_ratio,
                                       ((depth * 1.0 / total_depth_size) - ((camera_distance - min_mm)/total_depth_size))  / (width_mm / total_depth_size));
                    pcl::transformPoint(v3, v3, camrot_mat);
                    pcl::transformPoint(v3, v3, rot_mat);


                    pt.x = v3[0];
                    pt.y = v3[1];
                    pt.z = v3[2];
                }
            }
        }
    }
}

void reconstruction3D::calculateRealPointCloud(std::vector<cv::Mat> depthmaps){
    /*
    // set initial settings of point cloud
    cloud->width    = depthmaps[0].cols;
    cloud->height   = depthmaps[0].rows;
    cloud->is_dense = false;
    int cn = depthmaps[0].channels();
    double aspect_ratio = cloud->width * 1.0f / cloud->height;
    double height_half = cloud->height*0.5;
    double width_half = cloud->width*0.5;
    double min_mm = 265.0; // 100.0f;
    double max_mm = 420.0; // 300.0f;
    double total_depth_size = max_mm - min_mm; //mm - maximum - minimum
    double fovy = 21.532 * M_PI / 180.0; // ;42.654
    double fovx = 28.452 * M_PI / 180.0; // 55.000
    double radius = 190.0 * 1.81949; // 1
    double cam_height = 75.979; //100.0;
    double camera_distance = std::sqrt(std::pow(radius,2) + std::pow(cam_height,2));
    double width_mm = std::tan(fovx*0.5) * camera_distance * 2;
    double height_mm = std::tan(fovy*0.5) * camera_distance * 2;

    // calculate size of cloud
    int cloud_size = 0;
    for(int y=0; y<cloud->height; y++){
        for(int x=0; x<cloud->width; x++){
            int pos = y*depthmaps[0].cols*cn + x*cn + 0;
            if (depthmaps[0].data[pos] > 0){
                cloud_size++;
            }
        }
    }
    cloud->points.resize(cloud_size);

    // calculate first cloud
    int depth_idx = 0;
    for(int y=0; y<cloud->height; y++){
        for(int x=0; x<cloud->width; x++){
            int pos = y*cloud->width*cn + x*cn + 0;
            if (depthmaps[0].data[pos] > 0){
                pcl::PointXYZ& pt = cloud->points[depth_idx++];
                double pixel_value = depthmaps[0].data[pos];
                double d = pixel_value / 255.0; // [0,1]
                Eigen::Vector2d xy(std::abs(width_half - x)  / cloud->width * width_mm,
                                   std::abs(height_half - y) / cloud->height * height_mm);

                double hypo = d*total_depth_size + min_mm;
                double z = std::sqrt(std::abs(std::pow(hypo,2) - std::pow(xy.norm(),2))); // calculate z!!!

                // a² = c² - b²
                double depth = (z - min_mm);
                Eigen::Vector3f v3((x * 1.0 / cloud->width  - 0.5),
                                    -(y * 1.0 / cloud->height - 0.5) / aspect_ratio,
                                   ((depth * 1.0 / total_depth_size) -
                                    ((camera_distance - min_mm)/total_depth_size))  / (width_mm / total_depth_size));

                v3 = v3*10;

                pt.x = v3[0];
                pt.y = v3[1];
                pt.z = v3[2];
            }
        }
    }


    for (int i=1; i<depthmaps.size(); i++){
        // set initial settings of point cloud
        pcl::PointCloud<pcl::PointXYZ>::Ptr cloudi (new pcl::PointCloud<pcl::PointXYZ>);
        cloudi->width    = depthmaps[i].cols;
        cloudi->height   = depthmaps[i].rows;
        cloudi->is_dense = false;
        //int cn = depthmaps[i].channels();
        //double min_mm = 100.0;// 155.0f;
        //double max_mm = 300.0; //300.0f;
        //double total_depth_size = max_mm - min_mm; //mm - maximum - minimum
        //double fovy = 42.654 * M_PI / 180.0;
        //double fovx = 55.000 * M_PI / 180.0;
        //double width_mm = std::tan(fovx*0.5) * camera_distance * 2;
        //double height_mm = std::tan(fovy*0.5) * camera_distance * 2;

        // calculate size of cloud
        int cloudi_size = 0;
        for(int y=0; y<cloudi->height; y++){
            for(int x=0; x<cloudi->width; x++){
                int pos = y*depthmaps[i].cols*cn + x*cn + 0;
                if (depthmaps[i].data[pos] > 0){
                    cloudi_size++;
                }
            }
        }

        cloudi->points.resize(cloudi_size);


        depth_idx = 0;
        for(int y=0; y<cloudi->height; y++){
            for(int x=0; x<cloudi->width; x++){
                int pos = y*depthmaps[i].cols*cn + x*cn + 0;
                if (depthmaps[i].data[pos] > 0){
                    pcl::PointXYZ& pt = cloudi->points[depth_idx++];
                    double pixel_value = depthmaps[i].data[pos];
                    double d = 1.0 - pixel_value / 255.0; // [0,1]
                    Eigen::Vector2d xy(std::abs(width_half - x)  / cloudi->width * width_mm,
                                       std::abs(height_half - y) / cloudi->height * height_mm);

                    double hypo = d*total_depth_size + min_mm;
                    double z = std::sqrt(std::abs(std::pow(hypo,2) - std::pow(xy.norm(),2))); // calculate z!!!

                    // a² = c² - b²
                    double depth = (z - min_mm);
                    Eigen::Vector3f v3((x * 1.0 / cloudi->width  - 0.5),
                                        -(y * 1.0 / cloudi->height - 0.5) / aspect_ratio,
                                       ((depth * 1.0 / total_depth_size) - ((camera_distance - min_mm)/total_depth_size))  / (width_mm / total_depth_size));


                    v3 = v3*10;
                    pt.x = v3[0];
                    pt.y = v3[1];
                    pt.z = v3[2];
                }
            }
        }

        // The Iterative Closest Point algorithm
        int iterations = 50;
        pcl::IterativeClosestPoint<pcl::PointXYZ, pcl::PointXYZ> icp;
        // Set the max correspondence distance to 5cm (e.g., correspondences with higher distances will be ignored)
        //icp.setMaxCorrespondenceDistance (0.05);
        // Set the maximum number of iterations (criterion 1)
        icp.setMaximumIterations (iterations);
        // Set the transformation epsilon (criterion 2)
        //icp.setTransformationEpsilon (1e-8);
        // Set the euclidean distance difference epsilon (criterion 3)
        //icp.setEuclideanFitnessEpsilon (1);
        icp.setInputSource(cloudi);
        icp.setInputTarget(cloud);
        pcl::PointCloud<pcl::PointXYZ> Final;
        icp.align(Final);
        //icp.align(*cloud);

        if (icp.hasConverged ())
        {
            std::cout << "has converged!" << std::endl;
            //*cloud = *cloudi;
            *cloud += Final;

            Eigen::Matrix4d transformation_matrix = Eigen::Matrix4d::Identity ();
            transformation_matrix = icp.getFinalTransformation ().cast<double>();
            //print4x4Matrix (transformation_matrix);
        }
        else
        {
            std::cout << "ERROR: ICP did not converge!!" << std::endl;
            PCL_ERROR ("\nICP has not converged.\n");
            //return (-1);
        }
    }*/

    // set initial settings of point cloud
    cloud->width    = depthmaps[0].cols;
    cloud->height   = depthmaps[0].rows;
    cloud->is_dense = false;

    //unsigned short max_v = 17200;
    //unsigned short min_v = 14000;

    // calculate size of cloud
    int cloud_size = 0;
    for (int i=0; i<depthmaps.size(); i++){
        for(int y=0; y<cloud->height; y++){
            for(int x=0; x<cloud->width; x++){
                unsigned short pixel_value = depthmaps[i].at<unsigned short>(y,x);
                if (pixel_value > 0){
                    cloud_size++;
                }
            }
        }
    }

    // set size of cloud
    cloud->points.resize(cloud_size);


    // Fill in the cloud data
    // teapot width = 85mm // 195
    double radius = 190.0 * 1.81949; // 1
    double cam_height = 75.979 - 47.956; //147.956 - 47.956;
    double aspect_ratio = cloud->width * 1.0f / cloud->height;
    double camera_distance = std::sqrt(std::pow(radius,2) + std::pow(cam_height,2));
    double rot_cam = std::atan(cam_height / radius);
    double min_mm = 265.0; // 100.0f;
    double max_mm = 420.0; // 300.0f;
    double total_depth_size = max_mm - min_mm; //mm - maximum - minimum
    double fovy = 21.532 * M_PI / 180.0; // ;42.654
    double fovx = 28.452 * M_PI / 180.0; // 55.000
    double degree = 360.0 / 36.0;
    double width_mm = std::tan(fovx*0.5) * camera_distance * 2;
    double height_mm = std::tan(fovy*0.5) * camera_distance * 2;
    double height_half = cloud->height*0.5;
    double width_half = cloud->width*0.5;
    int depth_idx = 0;
    std::set<unsigned short> grayscale_count;

    Eigen::Affine3f camrot_mat = Eigen::Affine3f::Identity();
    camrot_mat.rotate (Eigen::AngleAxisf (rot_cam, Eigen::Vector3f::UnitX()));

    // GAMMA was activated in 3DsMax :C!!!
    double bitdepth = std::pow(2, 16);

    double max_v = 17100*2;
    double min_v = 14800*2;
    double range_v = max_v-min_v;

    for (int i=0; i<depthmaps.size(); i++){
        Eigen::Affine3f rot_mat = Eigen::Affine3f::Identity();
        rot_mat.rotate (Eigen::AngleAxisf (-degree*i * M_PI / 180.0, Eigen::Vector3f::UnitY()));

        for(int y=0; y<cloud->height; y++){
            for(int x=0; x<cloud->width; x++){
                if (depthmaps[i].at<unsigned short>(y,x) > 0){
                    pcl::PointXYZ& pt = cloud->points[depth_idx++];
                    unsigned short pixel_value = depthmaps[i].at<unsigned short>(y,x);
                    grayscale_count.insert(pixel_value);
                    double d = ((pixel_value - min_v)*1.0 - range_v*1.0 / 2.0) / range_v * 0.5687;
                    //pixel_value -= (min_v + (max_v - min_v) / 2);
                    //double d = pixel_value * 1.0 / bitdepth; // [0,1]
                    /*Eigen::Vector2d xy(std::abs(width_half - x)  / cloud->width * width_mm,
                                       std::abs(height_half - y) / cloud->height * height_mm);

                    double hypo = d*total_depth_size + min_mm;
                    double z = std::sqrt(std::abs(std::pow(hypo,2) - std::pow(xy.norm(),2))); // calculate z!!!

                    // a² = c² - b²
                    double depth = (z - min_mm);*/
                    Eigen::Vector3f v3((x * 1.0 / cloud->width  - 0.5),
                                        -(y * 1.0 / cloud->height - 0.5) / aspect_ratio,
                                       d);
                    //pcl::transformPoint(v3, v3, camrot_mat);
                    //pcl::transformPoint(v3, v3, rot_mat);


                    pt.x = v3[0];
                    pt.y = v3[1];
                    pt.z = v3[2];
                }
            }
        }
    }

    std::cout << "number of different values: " << grayscale_count.size() << std::endl;
    //std::cout << "min value: " << grayscale_count. << std::endl;
    //std::cout << "max value: " << grayscale_count.end() << std::endl;

    // set size of cloud
}

void reconstruction3D::approximateNormals(){
    if (cloud->empty())
        return;
    // Create a KD-Tree
    //pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ>);
    //cloud_normals = pcl::PointCloud<pcl::PointNormal>::Ptr(new pcl::PointCloud<pcl::PointNormal>);

    // Init object (second point type is for the normals, even if unused)
    pcl::MovingLeastSquares<pcl::PointXYZ, pcl::PointXYZ> mls;

    mls.setComputeNormals (true);

    // Set parameters
    mls.setInputCloud (cloud);
    mls.setSearchRadius (0.03);
    mls.setPolynomialFit (true);

    mls.setPolynomialOrder (2);
    mls.setUpsamplingMethod (pcl::MovingLeastSquares<pcl::PointXYZ, pcl::PointXYZ>::SAMPLE_LOCAL_PLANE);
    //mls.setUpsamplingMethod(pcl::MovingLeastSquares<pcl::PointXYZ,pcl::PointXYZ>::VOXEL_GRID_DILATION);
    mls.setDilationVoxelSize(0.005); // none
    mls.setUpsamplingRadius (0.005); // 0.005 : higher -> fills up holes
    mls.setUpsamplingStepSize (0.003); // 0.003

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_smoothed (new pcl::PointCloud<pcl::PointXYZ> ());
    mls.process (*cloud_smoothed);

    pcl::NormalEstimationOMP<pcl::PointXYZ, pcl::Normal> ne;
    ne.setNumberOfThreads (8);
    ne.setInputCloud (cloud_smoothed);
    ne.setRadiusSearch (0.25); // 0.01 : higher -> smoother?!
    Eigen::Vector4f centroid;
    compute3DCentroid (*cloud_smoothed, centroid);
    ne.setViewPoint (centroid[0], centroid[1], centroid[2]);

    pcl::PointCloud<pcl::Normal>::Ptr cloud_normals (new pcl::PointCloud<pcl::Normal> ());
    ne.compute (*cloud_normals);

    /*for (size_t i = 0; i < cloud_normals->size (); ++i)
    {
     cloud_normals->points[i].normal_x *= -1;
     cloud_normals->points[i].normal_y *= -1;
     cloud_normals->points[i].normal_z *= -1;
    }*/
    //cloud_smoothed_normals = pcl::PointCloud<pcl::PointNormal>::Ptr (new pcl::PointCloud<pcl::PointNormal> ());
    //concatenateFields (*cloud_smoothed, *cloud_normals, *cloud_smoothed_normals);
}

void reconstruction3D::calculateMesh(){


    pcl::search::KdTree<pcl::PointNormal>::Ptr tree2 (new pcl::search::KdTree<pcl::PointNormal>);
    //tree2->setInputCloud (cloud_smoothed_normals);


    // Initialize objects
    pcl::GreedyProjectionTriangulation<pcl::PointNormal> gp3;

    // Set the maximum distance between connected points (maximum edge length)
    gp3.setSearchRadius (0.025); // 0.025

    // Set typical values for the parameters
    gp3.setMu (std::max(this->mesh_mu,12.5)); // 2.5
    gp3.setMaximumNearestNeighbors (100);   // 100
    gp3.setMaximumSurfaceAngle(M_PI/180.0f * 45); // 90 degrees, before 45
    gp3.setMinimumAngle(M_PI/180.0f * 10); // 10 degrees
    gp3.setMaximumAngle(M_PI/180.0f * 120); // 90 degrees, before 120
    gp3.setNormalConsistency(false);

    // Get result
    //gp3.setInputCloud (cloud_smoothed_normals);
    gp3.setSearchMethod (tree2);
    gp3.reconstruct (triangles);

    /*
    // Poisson always creates closed/"watertight" meshes
    if (cloud_smoothed_normals->empty())
        return;

    pcl::Poisson<pcl::PointNormal> poisson;
    poisson.setDepth (9);
    poisson.setInputCloud(cloud_smoothed_normals);
    poisson.reconstruct (triangles);
    */
}

void reconstruction3D::showPointCloud(){
    pcl::visualization::CloudViewer cloud_viewer("Simple Cloud Viewer");
    is_viewing = false;
    cloud_viewer.showCloud (cloud);
    is_viewing = true;
    while (!cloud_viewer.wasStopped() && is_viewing)
    {

    }
    is_viewing = false;
}

void reconstruction3D::saveCloudToPLY(){
    if (!cloud->empty()){
        QString filename = QFileDialog::getSaveFileName(0,
                                                    "Save File",
                                                    QDir::currentPath(),
                                                    "Point Clouds (*.ply)");
        pcl::io::savePLYFile(filename.toStdString(), *cloud, false); // binary mode?

        saveToPly(filename);
    }
}

void reconstruction3D::showMesh(){
    // Additional vertex information
    //std::vector<int> parts = gp3.getPartIDs();
    //std::vector<int> states = gp3.getPointStates();
    //viewer.removeAllShapes();

    /*pcl::visualization::CloudViewer viewer ("Simple Cloud Viewer");
    viewer.showCloud (cloud);
    while (!viewer.wasStopped ())
       {
       }
    */

    pcl::visualization::PCLVisualizer* viewer;
    viewer = new pcl::visualization::PCLVisualizer("3D Viewer");
    viewer->addPolygonMesh(triangles);
    while (!viewer->wasStopped ()){
        viewer->spinOnce ();
        Sleep(100);
    }
    HWND hWnd = (HWND)viewer->getRenderWindow()->GetGenericWindowId();
    delete viewer;
    DestroyWindow(hWnd);
    is_viewing = false;
}

void reconstruction3D::setMeshMu(double mu){
    this->mesh_mu = mu;
}

void reconstruction3D::saveToPly(QString filename){
    QFile f(filename);
    if (f.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&f);
        out << "ply\n";
        out << "format ascii 1.0\n";
        out << "comment PCL generated\n";
        out << "element vertex " << cloud->points.size() << "\n";
        out << "property float x\n";
        out << "property float y\n";
        out << "property float z\n";
        out << "element camera 1\n";
        out << "property float view_px\n";
        out << "property float view_py\n";
        out << "property float view_pz\n";
        out << "property float x_axisx\n";
        out << "property float x_axisy\n";
        out << "property float x_axisz\n";
        out << "property float y_axisx\n";
        out << "property float y_axisy\n";
        out << "property float y_axisz\n";
        out << "property float z_axisx\n";
        out << "property float z_axisy\n";
        out << "property float z_axisz\n";
        out << "property float focal\n";
        out << "property float scalex\n";
        out << "property float scaley\n";
        out << "property float centerx\n";
        out << "property float centery\n";
        out << "property int viewportx\n";
        out << "property int viewporty\n";
        out << "property float k1\n";
        out << "property float k2\n";
        out << "end_header\n";

        int depth_idx = 0;

        for (int i=0; i < cloud->points.size(); i++){
            pcl::PointXYZ& pt = cloud->points[depth_idx++];
            out << pt.x << ' ' << pt.y << ' ' << pt.z << '\n';
        }
        // view: xyz, x_axis, y_axis, z-axis, focal, scale xy, center xy,
        out << "0 0 0 1 0 0 0 1 0 0 0 1 0 0 0 0 0 ";
        // viewport xy
        out << cloud->width << " ";
        out << cloud->height << " ";
        out << "0 0\n";
    }


    f.close();
}
