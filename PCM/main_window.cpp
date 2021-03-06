#include "main_window.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QLabel>
#include <QStatusBar>
#include <QSettings>
#include <QCloseEvent>
#include <QPlainTextEdit>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <QGroupBox>
#include <QColorDialog>
#include <QComboBox>
#include "file_system.h"
#include "sample.h"
#include "sample_set.h"
#include "file_io.h"
#include "color_table.h"
#include "time.h"
#include "trajectory_classifier.h"
#include "tracer.h"
#include "spectral_clustering.h"
#include "scanner.h"
#include "show_normal_tool.h"
#include "sample_properity.h"
#include "traj_clustering.h"

using namespace qglviewer;
using namespace std;

main_window::main_window(QWidget *parent)
	: QMainWindow(parent),
	cur_select_sample_idx_(-1),
	last_select_sample_idx_(-1)
{


	ui.setupUi(this);

	QGLFormat format = QGLFormat::defaultFormat();
	format.setSampleBuffers(true);
	format.setSamples(8);
	main_canvas_ = new PaintCanvas(format, this);

	setCentralWidget(main_canvas_);

	setWindowTitle("PCM");

	setContextMenuPolicy(Qt::CustomContextMenu);
//	setWindowState(Qt::WindowMaximized);

	setFocusPolicy(Qt::ClickFocus);

	main_canvas_->camera()->setType(Camera::PERSPECTIVE);

	//Codes for initializing treeWidget
	QStringList headerLabels;
	headerLabels <<"Index"<<  "Label" << "#Point";
	ui.treeWidget->setHeaderLabels(headerLabels);
	ui.treeWidget->setColumnWidth( 0,50 );
	connect(ui.treeWidget, SIGNAL(itemClicked( QTreeWidgetItem * , int  ) ),
		this, SLOT(selectedSampleChanged( QTreeWidgetItem * , int  )) );


	createAction();
	createStatusBar();

	//srand(time(NULL));

}


void main_window::resetSampleSet()
{
	cur_import_files_attr_.clear();
	cur_select_sample_idx_ = last_select_sample_idx_ = -1;
	SampleSet::get_instance().clear();
}

void main_window::createAction()
{
	createFileMenuAction();
	createPaintSettingAction();
	createAlgorithmAction();
	createToolAction();

	connect(ui.actionScanner,SIGNAL(triggered()), this, SLOT(openScanner()));
	connect( ui.actionShow_Selected_Trajectory, SIGNAL(triggered()), this, SLOT(showSelectedTrajectory()) );

}


void main_window::createAlgorithmAction()
{
	connect(ui.actionClustering, SIGNAL(triggered()), this, SLOT(doSpectralClustering()));
	connect(ui.actionTraj_Clustering, SIGNAL(triggered()), this, SLOT(doTrajClustering()));

}

void main_window::createPaintSettingAction()
{
	connect(ui.actionSet_Visible, SIGNAL(triggered()), this, SLOT(setSampleVisible()) );
	connect(ui.actionSet_Invisible, SIGNAL(triggered()), this, SLOT(setSampleInvisible()));
	connect(ui.actionObject_Color, SIGNAL(triggered()), this, SLOT(setObjectColorMode()));
	connect(ui.actionVertex_Color, SIGNAL(triggered()), this, SLOT(setVertexColorMode()));
	connect(ui.actionLabel_Color, SIGNAL(triggered()), this, SLOT(setLabelColorMode()));
	connect(ui.actionShow_Tracjectory, SIGNAL(triggered()), this, SLOT(showTracer()));
	connect(ui.actionDont_Trace,SIGNAL(triggered()), this, SLOT(clearTracer()));
}

void main_window::createToolAction()
{
	connect( ui.actionSelect_Mode, SIGNAL(triggered()), this, SLOT(setSelectToolMode()) );
	connect( ui.actionScene_Mode, SIGNAL(triggered()),this, SLOT(setSceneToolMode()));
	connect( ui.actionShow_Normal, SIGNAL(triggered()), this, SLOT(setNormalToolMode()) );
	connect( ui.actionCompute_Normal, SIGNAL(triggered()), this, SLOT(computeSampleNormal()) );

}

void main_window::setObjectColorMode()
{
	main_canvas_->which_color_mode_ = PaintCanvas::OBJECT_COLOR;
	main_canvas_->updateGL();
}

void main_window::setVertexColorMode()
{
	main_canvas_->which_color_mode_ = PaintCanvas::VERTEX_COLOR;
	main_canvas_->updateGL();
}

void main_window::setLabelColorMode()
{
	main_canvas_->which_color_mode_ = PaintCanvas::LABEL_COLOR;
	main_canvas_->updateGL();
}

void main_window::setSelectToolMode()
{
	if (cur_select_sample_idx_==-1)
	{
		return;
	}

	if (main_canvas_->single_operate_tool_)
	{
		delete main_canvas_->single_operate_tool_;
	}
	main_canvas_->single_operate_tool_ = new SelectTool(main_canvas_);
	main_canvas_->single_operate_tool_->set_tool_type(Tool::SELECT_TOOL);
	main_canvas_->single_operate_tool_->set_cur_smaple_to_operate(cur_select_sample_idx_);

	main_canvas_->updateGL();

}

void main_window::setNormalToolMode()
{
	if (cur_select_sample_idx_ == -1)
	{
		return;
	}
	if (main_canvas_->single_operate_tool_)
	{
		delete main_canvas_->single_operate_tool_;
	}
	main_canvas_->single_operate_tool_ = new ShowNormalTool(main_canvas_);
	main_canvas_->single_operate_tool_->set_tool_type(Tool::SHOW_NORMAL_TOOL);
	main_canvas_->single_operate_tool_->set_cur_smaple_to_operate( cur_select_sample_idx_ );

	main_canvas_->updateGL();
}

void main_window::setSceneToolMode()
{
	main_canvas_->single_operate_tool_->set_tool_type(Tool::EMPTY_TOOL);
	main_canvas_->updateGL();
}

void main_window::createFileMenuAction()
{
	connect(ui.actionImportFiles, SIGNAL(triggered()),this, SLOT(openFiles()));
	connect(ui.actionSaveFiles,SIGNAL(triggered()), this, SLOT(saveFiles()) );
}

bool main_window::openFile()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Import point cloud from file"), ".",
		tr(
		"Ascii Point cloud (*.lab *.xyz *.pwn *.pcd)\n"
		"All files (*.*)")
		);

	if (fileName.isEmpty())
		return false;

	return true;
}


bool main_window::setSampleVisible()
{
	SampleSet::get_instance()[cur_select_sample_idx_].set_visble(true);
	main_canvas_->updateGL();
	return true;
}

bool main_window::setSampleInvisible()
{
	SampleSet::get_instance()[cur_select_sample_idx_].set_visble(false);
	main_canvas_->updateGL();
	return true;
}

void main_window::selectedSampleChanged(QTreeWidgetItem * item, int column)
{
	last_select_sample_idx_ = cur_select_sample_idx_;
	cur_select_sample_idx_ = item->text( 0 ).toInt();

	if (last_select_sample_idx_!= -1)
	{
		SampleSet::get_instance()[last_select_sample_idx_].set_selected(false);
	}
	if ( cur_select_sample_idx_ != -1)
	{
		SampleSet::get_instance()[cur_select_sample_idx_].set_selected(true);
	}
	main_canvas_->updateGL();

}

void main_window::openScanner()
{
	Scanner* scanner_thread = new Scanner;
	connect(scanner_thread, SIGNAL(finished()), scanner_thread, SLOT(deleteLater() ));
	connect(scanner_thread, SIGNAL(finished_scan()), this, SLOT(closeScanner()) );
	scanner_thread->start();

}

void main_window::closeScanner()
{
	createTreeWidgetItems();
}

void main_window::showSelectedTrajectory()
{
	main_canvas_->showSelectedTraj();
}

void main_window::showTracer()
{
	main_canvas_->setTracerShowOrNot(true);
	main_canvas_->updateGL();
}

void main_window::clearTracer()
{
	main_canvas_->setTracerShowOrNot(false);
	Tracer::get_instance().clear_records();
	main_canvas_->updateGL();
}

bool main_window::openFiles()
{
	QString dir = QFileDialog::getExistingDirectory(this,tr("Import point cloud files"),".");
	if (dir.isEmpty())
		return false;

	resetSampleSet();

	QDir file_dir(dir);
	if ( !file_dir.exists() )
	{
		return false;
	}
	file_dir.setFilter(QDir::Files);

	QFileInfoList file_list = file_dir.entryInfoList();
	IndexType sample_idx = 0;
	for (IndexType file_idx = 0; file_idx < file_list.size(); file_idx++)
	{
		QFileInfo file_info = file_list.at(file_idx);
		FileIO::FILE_TYPE file_type;

		if (file_info.suffix() == "xyz")
		{
			file_type = FileIO::XYZ;
		}
		else
		{
			continue;
		}

		string file_path = file_info.filePath().toStdString();
		cur_import_files_attr_.push_back( make_pair(FileSystem::base_name(file_path), 
													FileSystem::extension(file_path)) );

		Sample* new_sample = FileIO::load_point_cloud_file(file_path, file_type,sample_idx);
		if (new_sample != nullptr)
		{
			SampleSet::get_instance().push_back(new_sample);
			sample_idx++;
		}

	}


	createTreeWidgetItems();

	return true;
}

bool main_window::saveFiles()
{
	QString dir = QFileDialog::getExistingDirectory(this,tr("Save point cloud files"),".");
	if (dir.isEmpty())
		return false;
	FileIO::save_point_cloud_to_file( dir.toStdString()+"\\", FileIO::XYZ );

}

void main_window::createStatusBar()
{
	coord_underMouse_label_ = new QLabel(this);
	coord_underMouse_label_->setAlignment(Qt::AlignLeft);

	vtx_idx_underMouse_label_ = new QLabel(this);
	coord_underMouse_label_->setAlignment(Qt::AlignRight);
	
	ui.statusBar->addWidget( coord_underMouse_label_ , 1 );
	ui.statusBar->addWidget( vtx_idx_underMouse_label_, 0 );
}

void main_window::computeSampleNormal()
{
	auto camera_look_at = main_canvas_->camera()->viewDirection();
	SampleManipulation::compute_normal( cur_select_sample_idx_ ,NormalType(camera_look_at.x, camera_look_at.y, camera_look_at.z));
}

void main_window::createTreeWidgetItems()
{
	ui.treeWidget->clear();
	
	SampleSet& set = SampleSet::get_instance();
	for ( int sample_idx=0; sample_idx < set.size(); sample_idx++ )
	{
		QTreeWidgetItem* item = new QTreeWidgetItem(ui.treeWidget); 


		ColorType color = set[ sample_idx ].color();

		item->setData(0, Qt::DisplayRole, sample_idx);
		item->setData(1,Qt::DecorationRole, QColor(color(0)*255, color(1)*255, color(2)*255) );
		item->setData(2, Qt::DisplayRole, set[sample_idx].num_vertices() );

		ui.treeWidget->insertTopLevelItem(sample_idx, item);
	}
}

void main_window::showCoordinateAndIndexUnderMouse( const QPoint& point )
{
	//Mouse point info come from canvas
	bool found = false;
	Vec v = main_canvas_->camera()->pointUnderPixel(point, found);
	if ( !found )
	{
		v = Vec();
	}
	QString coord_str = QString("XYZ = [%1, %2, %3]").arg(v.x).arg(v.y).arg(v.z);
	coord_underMouse_label_->setText(coord_str);

	IndexType idx;
	if ( !found || cur_select_sample_idx_==-1 )
	{
		idx = -1;
	}
	else
	{
		Sample& cur_selected_sample = SampleSet::get_instance()[cur_select_sample_idx_];
		Vec4 v_pre(v.x - Paint_Param::g_step_size(0)*cur_select_sample_idx_,
			v.y - Paint_Param::g_step_size(1)*cur_select_sample_idx_,
			v.z - Paint_Param::g_step_size(2)*cur_select_sample_idx_ ,1.);
		//Necessary to do this step, convert view-sample space to world-sample space
		v_pre = cur_selected_sample.matrix_to_scene_coord().inverse() * v_pre;
		idx = cur_selected_sample.closest_vtx( PointType(v_pre(0), v_pre(1), v_pre(2)) );
	}
	QString idx_str = QString("VERTEX INDEX = [%1]").arg(idx);
	vtx_idx_underMouse_label_->setText( idx_str );

	return;
}


void main_window::doSpectralClustering()
{
	SpectralClusteringThread* cluster = new SpectralClusteringThread;
	connect(cluster, SIGNAL(finished()), cluster, SLOT(deleteLater() ));
	cluster->start();

}

void main_window::doTrajClustering()
{
	TrajClusteringThread* cluster = new TrajClusteringThread;
	connect( cluster, SIGNAL(finished()), cluster, SLOT(deleteLater()) );
	cluster->start();

}


void main_window::finishClustering()
{

}

main_window::~main_window()
{
	SampleSet::get_instance().clear();


}
