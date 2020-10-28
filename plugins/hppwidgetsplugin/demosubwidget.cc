//
// Copyright (c) CNRS
// Authors: Yann de Mont-Marin
//
#include <QFileDialog>
#include <QInputDialog>
#include <QString>

#include <boost/lexical_cast.hpp> 
#include <boost/algorithm/string.hpp>

#include "hppwidgetsplugin/demosubwidget.hh"
#include <hppwidgetsplugin/hppwidgetsplugin.hh>
#include <gepetto/gui/mainwindow.hh>
#include "hpp/corbaserver/client.hh"


namespace hpp {
  namespace gui {

    using gepetto::gui::MainWindow;
    using CORBA::ULong;

    DemoSubWidget::DemoSubWidget(HppWidgetsPlugin *plugin):
      QObject (plugin),
      plugin_ (plugin)
    {
    }

    DemoSubWidget::~DemoSubWidget(){
      foreach (QAction* act, buttons_) {
        toolbar_->removeAction (act);
        delete act;
      }
    }

    void DemoSubWidget::init(){
      MainWindow* main = MainWindow::instance();
      toolbar_ = MainWindow::instance()->addToolBar("demo toolbar");
      toolbar_->setObjectName ("demo.toolbar");

      QAction* reset = new QAction ("Reset", toolbar_);
      toolbar_->addAction (reset);
      connect (reset, SIGNAL(triggered()), SLOT (resetAll()));
      main->registerSlot("reset", this);
      buttons_.append(reset);

      QAction* load = new QAction ("Load demo", toolbar_);
      toolbar_->addAction (load);
      connect (load, SIGNAL(triggered()), SLOT (loadDemo()));
      main->registerSlot("loadDemo", this);
      buttons_.append(load);

      QAction* save = new QAction ("Save demo", toolbar_);
      toolbar_->addAction (save);
      connect (save, SIGNAL(triggered()), SLOT (saveDemo()));
      main->registerSlot("saveDemo", this);
      buttons_.append(save);

      plugin_->client ()->problem ()->resetProblem ();
    }

    std::string DemoSubWidget::robotName(){
      CORBA::String_var robotName = plugin_->client ()->robot()->getRobotName();
      return (std::string) robotName.in();
    }

    void DemoSubWidget::resetAll(){
      MainWindow* main = MainWindow::instance ();

      // Reinitialize config_list
      clWidget()->reinitialize();

      // Reset HPP class
      plugin_->client ()->problem ()->resetProblem ();

      // Deletes all non default nodes
      std::vector<std::string> nodes(main->osg()->getNodeList());
      for(std::vector<std::string>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (!(*it == "gepetto-gui" || *it == "hpp-gui" ||  *it == "joints" || (*it).substr(0, 4) == "View")){
          main->osg()->deleteNode(*it, 1);
        }
      }

      // Request refresh of the main window
      main->requestRefresh();
    }

    void DemoSubWidget::loadDemo(){
      MainWindow* main = MainWindow::instance();

      QString filename = QFileDialog::getOpenFileName (NULL, "Select a demo file");
      if (filename.isNull()) return;
      std::string filename_ (filename.toStdString());

      TiXmlDocument doc(filename_);
      if(!doc.LoadFile()){
        const char* msg = "Unable to load demo file";
        if (main != NULL)
          main->logError(msg);
        return;
      }

      TiXmlHandle hdl(&doc);
      TiXmlElement* problem_element = hdl.FirstChildElement().Element();

      // Check that robot match
      std::string root_type = problem_element->ValueStr ();
      std::string robot_name = problem_element->Attribute("robotname");
      if (
        root_type.compare("problem")!=0 || robot_name.compare(robotName())!=0
      ){
        const char* msg = "The robot and the demo file does not match";
        if (main != NULL)
          main->logError(msg);
        return;
      }

      // Loop on all elements
      TiXmlElement *elem = hdl.FirstChildElement().FirstChildElement().Element();
      while (elem){
        // Get name
        std::string name = elem->Attribute("name");

        // Parse floatSeq
        std::vector<std::string> parsing;
        std::string txt = elem->GetText();
        boost::split(parsing, txt, boost::is_any_of(" "));

        hpp::floatSeq* fS = new hpp::floatSeq();
        fS->length((ULong) parsing.size());
        for(size_t i=0; i<parsing.size(); i++){
          (*fS)[(ULong) i] = boost::lexical_cast<double> (parsing[i]);
        }

        // Get element type
        std::string type = elem->ValueStr ();

        if (type.compare("joint") == 0){
          loadBound(name, *fS);
          delete fS; // We only use fS to update bounds
        }
        else if (type.compare("config") == 0){
          loadConfig(name, *fS);
        }
        elem = elem->NextSiblingElement(); // iteration 
      }
      MainWindow::instance()->requestRefresh(); // To make new bounded item slider
    }

    void DemoSubWidget::saveDemo(){
      QString filename = QFileDialog::getSaveFileName(NULL, tr("Select a destination"));
      if (filename.isNull()) return;
      std::string filename_ (filename.toStdString());

      TiXmlDocument doc;

      TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "", "" );
      doc.LinkEndChild( decl );

      TiXmlElement* problem_element = new TiXmlElement("problem");
      problem_element->SetAttribute("robotname", robotName());
      doc.LinkEndChild( problem_element );

      writeBounds(problem_element);

      writeConfigs(problem_element);

      doc.SaveFile(filename_.c_str());
    }

    void DemoSubWidget::loadBound(const std::string & name, const hpp::floatSeq & fS){
      CORBA::Long nbDof = plugin_->client()->robot ()->getJointNumberDof(name.c_str());
      if (nbDof > 0) {
        hpp::floatSeq_var bounds =
          plugin_->client()->robot()->getJointBounds(name.c_str());
        hpp::floatSeq& b = bounds.inout();
        for (size_t i=0; i < b.length (); ++i){
          b[(ULong) i] = fS[(ULong) i];
        }
      plugin_->client()->robot()->setJointBounds(name.c_str(), b);
      }
    }

    void DemoSubWidget::loadConfig(const std::string & name, const hpp::floatSeq & fS){
      clWidget()->reciveConfig(QString::fromUtf8(name.c_str()), fS);
    }

    void DemoSubWidget::writeBounds(TiXmlElement * const parent){
      hpp::Names_t_var joints = plugin_->client()->robot()->getAllJointNames ();
      for (size_t i = 0; i < joints->length (); ++i) {
        const char* jointName = joints[(ULong) i];
        CORBA::Long nbDof = plugin_->client()->robot ()->getJointNumberDof(jointName);
        if (nbDof > 0) {
          hpp::floatSeq_var bounds =
            plugin_->client()->robot()->getJointBounds(jointName);
        
          const hpp::floatSeq& b = bounds.in();
          writeElement(parent, "joint",
                       (std::string) jointName,
                       b);
        }
      }
    }

    void DemoSubWidget::writeConfigs(TiXmlElement * const parent){
      clWidget()->resetAllConfigs(); // We pull all config in list
      for (int i = 0; i < clWidget()->list()->count(); ++i){
        QListWidgetItem* item = clWidget()->list()->item(i);
        writeElement(parent, "config",
                     item->text().toStdString(),
                     clWidget()->getConfig(item));
      }
    }

    void DemoSubWidget::writeElement(TiXmlElement * const parent,
                                           const std::string & type,
                                           const std::string & name,
                                           const hpp::floatSeq & fS){
      // Convert hpp floafSeq
      std::string textual_seq("");
      for (size_t i=0; i< fS.length(); ++i){
        textual_seq += boost::lexical_cast<std::string>(fS[(ULong) i]);
        if (i != fS.length() -1){
          textual_seq += " ";
        }
      }
      // Create node
      TiXmlElement * element = new TiXmlElement(type);
      element->SetAttribute("name", name );
      TiXmlText * text = new TiXmlText(textual_seq);
      element->LinkEndChild(text);
      parent->LinkEndChild(element);
    }

    ConfigurationListWidget* DemoSubWidget::clWidget() const {
      return plugin_->configurationListWidget();
    }
  } // namespace gui
} // namespace hpp