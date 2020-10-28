//
// Copyright (c) CNRS
// Authors: Yann de Mont-Marin
//

#ifndef HPP_GUI_DEMOSUBWIDGET_HH
#define HPP_GUI_DEMOSUBWIDGET_HH

# include <string>
# include <tinyxml.h>

#include <QAction>
#include <QList>
#include <QToolBar>

#include <hppwidgetsplugin/hppwidgetsplugin.hh>
#include <hppwidgetsplugin/configurationlistwidget.hh>

namespace hpp {
  namespace gui {
    class DemoSubWidget : public QObject
    {
      Q_OBJECT

      public:
        explicit DemoSubWidget(HppWidgetsPlugin *plugin);
        virtual ~DemoSubWidget();
        void init();

      public slots:
        void loadDemo();
        void saveDemo();
        void resetAll();

      protected:
        std::string robotName();

        void writeBounds(TiXmlElement * const parent);
        void writeConfigs(TiXmlElement * const parent);
        void writeElement(TiXmlElement * const parent, const std::string & type, const std::string & name, const hpp::floatSeq & fS);

        void loadConfig(const std::string & name, const hpp::floatSeq & fS);
        void loadBound(const std::string & name, const hpp::floatSeq & fS);

      private:
        QList <QAction*> buttons_;
        QToolBar* toolbar_;
        HppWidgetsPlugin* plugin_;
        ConfigurationListWidget* clWidget() const;
    };
  } // namespace gui
} // namespace hpp

#endif // HPP_GUI_DEMOSUBWIDGET_HH