/*=========================================================================
  Program:   Multimod Application Framework
  Module:    $RCSfile: medOpExporterGRFWS.cpp,v $
  Language:  C++
  Date:      $Date: 2010-10-07 10:02:41 $
  Version:   $Revision: 1.1.2.10 $
  Authors:   Simone Brazzale
==========================================================================
  Copyright (c) 2001/2005 
  CINECA - Interuniversity Consortium (www.cineca.it)
=========================================================================*/

#include "mafDefines.h" 
//----------------------------------------------------------------------------
// NOTE: Every CPP file in the MAF must include "mafDefines.h" as first.
// This force to include Window,wxWidgets and VTK exactly in this order.
// Failing in doing this will result in a run-time error saying:
// "Failure#0: The value of ESP was not properly saved across a function call"
//----------------------------------------------------------------------------
#include "medOpExporterGRFWS.h"

#include <wx/busyinfo.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include "mafGUI.h"
#include "mafTagArray.h"

#include <fstream>
#include <iostream>

#include <vtkCell.h>
#include <vtkPoints.h>
#include <vtkDataSet.h>
#include <vtkPolyData.h>

#include <mafTransformBase.h>
#include <mafMatrix.h>
#include <mafGUIRollOut.h>

using namespace std;

#define TAG_FORMAT "FORCE PLATES"
#define FREQ 1.00
#define DELTA 5.0

//----------------------------------------------------------------------------
medOpExporterGRFWS::medOpExporterGRFWS(const wxString &label) :
mafOp(label)
//----------------------------------------------------------------------------
{
	m_OpType	= OPTYPE_EXPORTER;
	m_Canundo	= true;
	m_File		= "";

  m_PlatformLeft = NULL;
  m_PlatformRight = NULL;
  m_ForceLeft = NULL;
  m_ForceRight = NULL;
  m_MomentLeft = NULL;
  m_MomentRight = NULL;
  m_Group = NULL;
  m_FastMethod = 0;
  m_Treshold = 10.0;
}
//----------------------------------------------------------------------------
medOpExporterGRFWS::~medOpExporterGRFWS()
//----------------------------------------------------------------------------
{
}
//----------------------------------------------------------------------------
void medOpExporterGRFWS::Clear()
//----------------------------------------------------------------------------
{
  m_PlatformLeft = NULL;
  m_PlatformRight = NULL;
  m_ForceLeft = NULL;
  m_ForceRight = NULL;
  m_MomentLeft = NULL;
  m_MomentRight = NULL;
}
//----------------------------------------------------------------------------
bool medOpExporterGRFWS::Accept(mafNode *node)
//----------------------------------------------------------------------------
{
  Clear();
  return (node && LoadVMEs(node));
}
//----------------------------------------------------------------------------
mafOp* medOpExporterGRFWS::Copy()   
//----------------------------------------------------------------------------
{
	medOpExporterGRFWS *cp = new medOpExporterGRFWS(m_Label);
	cp->m_File = m_File;
	return cp;
}
//----------------------------------------------------------------------------
void medOpExporterGRFWS::OpRun()   
//----------------------------------------------------------------------------
{
  if (!m_TestMode)
  {
    CreateGui();
    ShowGui();
  }
}
//----------------------------------------------------------------------------
void medOpExporterGRFWS::OpStop(int result)
//----------------------------------------------------------------------------
{
  HideGui();
  mafEventMacro(mafEvent(this,result));
}
//----------------------------------------------------------------------------
void medOpExporterGRFWS::CreateGui()
//----------------------------------------------------------------------------
{
  m_Gui = new mafGUI(this);
  m_Gui->SetOwnForegroundColour(wxColour(255,0,0));
  m_Gui->Label("This Operation takes seconds to hours to run", false);
  m_Gui->Label("depending on your input MSF file size. Running", false);
  m_Gui->Label("this Op after an Importer (C3D or Force Plates)", false);
  m_Gui->Label("will always take just few seconds!", false);
  m_Gui->Divider(0);
  m_Gui->Label("In any case, you can try the fast method which", false);
  m_Gui->Label("will guess the vector position time by time.", false);
  m_Gui->Label("This will reduce the time required but works", false);
  m_Gui->Label("only in case the input vectors", false);
  m_Gui->Label("DO NOT FLIP DIRECTIONS TOO", true);
  m_Gui->Label("QUICKLY!", true);

  m_Gui->Divider(0);
  m_Gui->Bool(-1,"Try fast method",&m_FastMethod,1,"Fast method");
  m_Gui->Divider(0);

  m_Gui->OkCancel();

  m_Gui->Divider(0);
  m_Gui->Divider(1);
  m_Gui->Divider(0);

  mafGUI* advanceSettings = new mafGUI(this);
  advanceSettings->Double(-1,"Treshold",&m_Treshold,0,10000,2,"Treshold");
  mafGUIRollOut* roll = new mafGUIRollOut(m_Gui,"Advance Settings",advanceSettings,-1,false); // set an id
}

//----------------------------------------------------------------------------
void medOpExporterGRFWS::OpDo()   
//----------------------------------------------------------------------------
{
  // Load and Execute
  if (LoadVMEs(m_Input))
  {
    wxString proposed = mafGetApplicationDirectory().c_str();
    proposed += "/Data/External/";
    proposed += m_Input->GetName();
    proposed += "_FORCEPLATES";
    proposed += ".csv";

    wxString wildc = "ASCII CSV file (*.csv)|*.csv";
    wxString f = mafGetSaveFile(proposed,wildc).c_str(); 

    if(!f.IsEmpty())
    {
      SetFileName(f.c_str());
      if (!m_FastMethod)
      {
        Write();
      }
      else
      {
        WriteFast();
      }
    }
  }
  else
  {
    wxMessageBox("Need 2 PLATFORMS, each with a FORCE vector and a MOMENT vector!","GRF Exporter Warning",wxOK | wxICON_ERROR);
    mafEventMacro(mafEvent(this,OP_RUN_CANCEL));
  }
  RemoveTempFiles();
}

//----------------------------------------------------------------------------
void medOpExporterGRFWS::OnEvent(mafEventBase *maf_event) 
//----------------------------------------------------------------------------
{
  if (mafEvent *e = mafEvent::SafeDownCast(maf_event))
  {
    switch(e->GetId())
    {
    case wxOK:
      OpStop(OP_RUN_OK);
      break;
    case wxCANCEL:
      OpStop(OP_RUN_CANCEL);
      break;
    default:
      mafEventMacro(*e);
    }
  }
}
//----------------------------------------------------------------------------
int medOpExporterGRFWS::LoadVMEs(mafNode* node)   
//----------------------------------------------------------------------------
{
  int result = 0;
  // Get input
  const mafNode::mafChildrenVector* children = node->GetChildren();
  for(int i = 0; i < children->size(); i++)
  {
    mafNode *child = children->at(i);
    if (child->IsA("mafVMESurface"))
    {
      if (m_PlatformLeft==NULL)
      {
        m_PlatformLeft = mafVMESurface::SafeDownCast(child);
      }
      else if (m_PlatformRight==NULL)
      {
        m_PlatformRight = mafVMESurface::SafeDownCast(child);
      }
    }
    if (child->IsA("mafVMEVector"))
    {
      if (m_ForceLeft==NULL)
      {
        m_ForceLeft = mafVMEVector::SafeDownCast(child);
      }
      else if (m_MomentLeft==NULL)
      {
        m_MomentLeft = mafVMEVector::SafeDownCast(child);
      }
      else if (m_ForceRight==NULL)
      {
        m_ForceRight = mafVMEVector::SafeDownCast(child);
      }
      else if (m_MomentRight==NULL)
      {
        m_MomentRight = mafVMEVector::SafeDownCast(child);
      }
    }
    if (m_PlatformLeft==NULL || m_PlatformRight==NULL || m_ForceLeft==NULL || m_ForceRight==NULL || m_MomentLeft==NULL || m_MomentRight==NULL)
    {
      result =  LoadVMEs(child);
    }
    else
    {
      return 1;
    }
  }
  return result;
}
//----------------------------------------------------------------------------
void medOpExporterGRFWS::Write()   
//----------------------------------------------------------------------------
{
  wxBusyInfo *wait = NULL;
  mafString info = "Loading data from files";
  if (!m_TestMode)
  {
    wxSetCursor(wxCursor(wxCURSOR_WAIT));
    mafEventMacro(mafEvent(this,PROGRESSBAR_SET_TEXT,&info));
	  mafEventMacro(mafEvent(this,PROGRESSBAR_SHOW));
    wait = new wxBusyInfo("This may take several minutes, please be patient...");
  }

  // Must update VTK!
  m_PlatformLeft->Update();
  m_PlatformLeft->GetVTKOutput()->Update();
  m_PlatformRight->Update();
  m_PlatformRight->GetVTKOutput()->Update();
  m_ForceLeft->Update();
  m_ForceLeft->GetVTKOutput()->Update();
  m_MomentLeft->Update();
  m_MomentLeft->GetVTKOutput()->Update();
  m_ForceRight->Update();
  m_ForceRight->GetVTKOutput()->Update();
  m_MomentRight->Update();
  m_MomentRight->GetVTKOutput()->Update();

  double bounds1[6];
  bounds1[0] = 0;
  m_PlatformLeft->GetOutput()->GetVTKData()->GetBounds(bounds1);
  bounds1[4] = bounds1[4] + DELTA;
  double bounds2[6];
  bounds2[0] = 0;
  m_PlatformRight->GetOutput()->GetVTKData()->GetBounds(bounds2);
  bounds2[4] = bounds2[4] + DELTA;

  wxString file1 = m_File + "_tmp1";
  wxString file2 = m_File + "_tmp2";
  wxString file3 = m_File + "_tmp3";
  wxString file4 = m_File + "_tmp4";

  std::ofstream f_Out(m_File);
  std::ofstream f_Out1(file1);
  std::ofstream f_Out2(file2);
  std::ofstream f_Out3(file3);
  std::ofstream f_Out4(file4);

  std::vector<mafTimeStamp> kframes1;
  std::vector<mafTimeStamp> kframes2;
  m_ForceLeft->GetTimeStamps(kframes1);
  m_ForceRight->GetTimeStamps(kframes2);
  std::vector<mafTimeStamp> kframes = MergeTimeStamps(kframes1,kframes2);
  int size = kframes.size();

  //Add times and values; time is always the first row
  double copL[3];   
  double copR[3];
  for (int i=0;i<size;i++)
  {
    double time = kframes.at(i);

    m_ForceLeft->SetTimeStamp(time);
    m_ForceLeft->Update();
    m_ForceLeft->GetVTKOutput()->Update();
      
    vtkPolyData* polyFL = (vtkPolyData*)m_ForceLeft->GetOutput()->GetVTKData();
     
    double* p = polyFL->GetPoint(0);
    copL[0] = p[0];
    copL[1] = p[1];
    copL[2] = p[2];
    f_Out1 << copL[0] << "," << copL[1] << "," << copL[2] << "\n";

    p = polyFL->GetPoint(1);
    double fL[3];
    fL[0] = p[0];
    fL[1] = p[1];
    fL[2] = p[2];
    f_Out1 << fL[0]-copL[0] << "," << fL[1]-copL[1] << "," << fL[2]-copL[2] << "\n";

    if (!m_TestMode)
    {
      mafEventMacro(mafEvent(this,PROGRESSBAR_SET_VALUE,(long)(((double) i)/((double) size)*25.)));
    }
  }
  for (int i=0;i<size;i++)
  {
    double time = kframes.at(i);

    m_MomentLeft->SetTimeStamp(time);
    m_MomentLeft->Update();
    m_MomentLeft->GetVTKOutput()->Update();
      
    vtkPolyData* polyML = (vtkPolyData*)m_MomentLeft->GetOutput()->GetVTKData();
      
    double* p = polyML->GetPoint(1);
    double mL[3];
    mL[0] = p[0];
    mL[1] = p[1];
    mL[2] = p[2];
    f_Out2 << mL[0]-copL[0] << "," << mL[1]-copL[1] << "," << mL[2]-copL[2] << "\n";

    if (!m_TestMode)
    {
      mafEventMacro(mafEvent(this,PROGRESSBAR_SET_VALUE,(long)(25+((double) i)/((double) size)*25.)));
    }
  }
  for (int i=0;i<size;i++)
  {
    double time = kframes.at(i);

    m_ForceRight->SetTimeStamp(time);
    m_ForceRight->Update();
    m_ForceRight->GetVTKOutput()->Update();
     
    vtkPolyData* polyFR = (vtkPolyData*)m_ForceRight->GetOutput()->GetVTKData();
      
    double* p = polyFR->GetPoint(0);
    copR[0] = p[0];
    copR[1] = p[1];
    copR[2] = p[2];
    f_Out3 << copR[0] << "," << copR[1] << "," << copR[2] << "\n";
      
    p = polyFR->GetPoint(1);
    double fR[3];
    fR[0] = p[0];
    fR[1] = p[1];
    fR[2] = p[2];
    f_Out3 << fR[0]-copR[0] << "," << fR[1]-copR[1] << "," << fR[2]-copR[2] << "\n";
      
    if (!m_TestMode)
    {
      mafEventMacro(mafEvent(this,PROGRESSBAR_SET_VALUE,(long)(50+((double) i)/((double) size)*25.)));
    }
  }
  for (int i=0;i<size;i++)
  {
    double time = kframes.at(i);

    m_MomentRight->SetTimeStamp(time);
    m_MomentRight->Update();
    m_MomentRight->GetVTKOutput()->Update();

    vtkPolyData* polyMR = (vtkPolyData*)m_MomentRight->GetOutput()->GetVTKData();
      
    double* p = polyMR->GetPoint(1);
    double mR[3];
    mR[0] = p[0];
    mR[1] = p[1];
    mR[2] = p[2];
    f_Out4 << mR[0]-copR[0] << "," << mR[1]-copR[1] << "," << mR[2]-copR[2] << "\n";

    if (!m_TestMode)
    {
      mafEventMacro(mafEvent(this,PROGRESSBAR_SET_VALUE,(long)(75+((double) i)/((double) size)*25.)));
    }
  }  

  f_Out1.close();
  f_Out2.close();
  f_Out3.close();
  f_Out4.close();

  wxFileInputStream inputFile1( file1 );
  wxTextInputStream text1( inputFile1 );
  wxFileInputStream inputFile2( file2 );
  wxTextInputStream text2( inputFile2 );
  wxFileInputStream inputFile3( file3 );
  wxTextInputStream text3( inputFile3 );
  wxFileInputStream inputFile4( file4 );
  wxTextInputStream text4( inputFile4 );

  wxString line;

  if (!m_TestMode)
  {
    mafEventMacro(mafEvent(this,PROGRESSBAR_SET_VALUE,(long)0.0));
  }

  if (!f_Out.bad())
  {
    // Add TAG
    f_Out << TAG_FORMAT << "\n";

    // Add the first row containing the frequency
    f_Out << FREQ << ",Hz\n";

    // Add a dummy line
    f_Out << "--- PLATES CORNERS SECTION ---\n";

    // Add the fourth line containing the plates tag
    f_Out << "Plate#,X1,Y1,Z1,X2,Y2,Z2,X3,Y3.Z3,X4,Y4,Z4\n";

    // Add the fifth and sixth line containing the corner values
    f_Out << "1,";
    
    f_Out << bounds1[0] << "," << bounds1[3] << "," << bounds1[4] << "," << bounds1[1] << "," << bounds1[3] << "," << bounds1[4] << ","
        << bounds1[1] << "," << bounds1[2] << "," << bounds1[4] << "," << bounds1[0] << "," << bounds1[2] << "," << bounds1[4] << "\n";
    f_Out << "2,";

    f_Out << bounds2[0] << "," << bounds2[3] << "," << bounds2[4] << "," << bounds2[1] << "," << bounds2[3] << "," << bounds2[4] << ","
        << bounds2[1] << "," << bounds2[2] << "," << bounds2[4] << "," << bounds2[0] << "," << bounds2[2] << "," << bounds2[4] << "\n";

    //Add a blank line 
    f_Out << "\n";

    // Add a dummy line
    f_Out << "--- FORCES VECTOR SECTION (1=PLATE1,2=PLATE2) ---\n";

    // Add a line containing the forces tag
    f_Out << "FRAME,COP1:X,COP1:Y,COP1:Z,Ref1:X,Ref1:Y,Ref1:Z,Force1:X,Force1:Y,Force1:Z,Moment1:X,Moment1:Y,Moment1:Z,COP2:X,COP2:Y,COP2:Z,Ref2:X,Ref2:Y,Ref2:Z,Force2:X,Force2:Y,Force2:Z,Moment2:X,Moment2:Y,Moment2:Z\n";

    //Add a blank line 
    f_Out << "\n";

    for (int i=0;i<size;i++)
    {
      double time = kframes.at(i);

      // TIME
      f_Out << time << ",";
        
      // COP L
      line = text1.ReadLine();
      f_Out << line << ",";
        
      // REF L
      f_Out << 0 << "," << 0 << "," << 0 << ",";
        
      // FORCE L
      line = text1.ReadLine();
      f_Out << line << ",";

      // MOMENT L
      line = text2.ReadLine();
      f_Out << line << ",";
              
      // COP R
      line = text3.ReadLine();
      f_Out << line << ",";
        
      // REF R
      f_Out << 0 << "," << 0 << "," << 0 << ",";
        
      // FORCE R
      line = text3.ReadLine();
      f_Out << line << ",";
      
      // MOMENT R
      line = text4.ReadLine();
      f_Out << line << "\n";

      if (!m_TestMode)
      {
        mafEventMacro(mafEvent(this,PROGRESSBAR_SET_VALUE,(long)(((double) i)/((double) size)*100.)));
      }

    }
    
    f_Out.close();
  }  

  info = "";
  if (!m_TestMode)
  {
    mafEventMacro(mafEvent(this,PROGRESSBAR_SET_TEXT,&info));
    mafEventMacro(mafEvent(this,PROGRESSBAR_HIDE));
    wxSetCursor(wxCursor(wxCURSOR_DEFAULT));
    cppDEL(wait);
  }
}
//----------------------------------------------------------------------------
void medOpExporterGRFWS::WriteFast()   
//----------------------------------------------------------------------------
{
  wxBusyInfo *wait = NULL;
  if (!m_TestMode)
  {
    wxSetCursor(wxCursor(wxCURSOR_WAIT));
	  mafEventMacro(mafEvent(this,PROGRESSBAR_SHOW));
    wait = new wxBusyInfo("This may take several minutes, please be patient!");
  }

  // Must update VTK!
  m_PlatformLeft->Update();
  m_PlatformLeft->GetVTKOutput()->Update();
  m_PlatformRight->Update();
  m_PlatformRight->GetVTKOutput()->Update();

  std::ofstream f_Out(m_File);
  if (!f_Out.bad())
  {
    // Add TAG
    f_Out << TAG_FORMAT << "\n";

    // Add the first row containing the frequency
    f_Out << FREQ << ",Hz\n";

    // Add a dummy line
    f_Out << "--- PLATES CORNERS SECTION ---\n";

    // Add the fourth line containing the plates tag
    f_Out << "Plate#,X1,Y1,Z1,X2,Y2,Z2,X3,Y3.Z3,X4,Y4,Z4\n";

    // Add the fifth and sixth line containing the corner values
    f_Out << "1,";
    double bounds[6];
    bounds[0] = 0;
    m_PlatformLeft->GetOutput()->GetVTKData()->GetBounds(bounds);
    bounds[4] = bounds[4] + DELTA;
    f_Out << bounds[0] << "," << bounds[3] << "," << bounds[4] << "," << bounds[1] << "," << bounds[3] << "," << bounds[4] << ","
        << bounds[1] << "," << bounds[2] << "," << bounds[4] << "," << bounds[0] << "," << bounds[2] << "," << bounds[4] << "\n";
    f_Out << "2,";
    m_PlatformRight->GetOutput()->GetVTKData()->GetBounds(bounds);
    bounds[4] = bounds[4] + DELTA;
    f_Out << bounds[0] << "," << bounds[3] << "," << bounds[4] << "," << bounds[1] << "," << bounds[3] << "," << bounds[4] << ","
        << bounds[1] << "," << bounds[2] << "," << bounds[4] << "," << bounds[0] << "," << bounds[2] << "," << bounds[4] << "\n";

    //Add a blank line 
    f_Out << "\n";

    // Add a dummy line
    f_Out << "--- FORCES VECTOR SECTION (1=PLATE1,2=PLATE2) ---\n";

    // Add a line containing the forces tag
    f_Out << "FRAME,COP1:X,COP1:Y,COP1:Z,Ref1:X,Ref1:Y,Ref1:Z,Force1:X,Force1:Y,Force1:Z,Moment1:X,Moment1:Y,Moment1:Z,COP2:X,COP2:Y,COP2:Z,Ref2:X,Ref2:Y,Ref2:Z,Force2:X,Force2:Y,Force2:Z,Moment2:X,Moment2:Y,Moment2:Z\n";

    //Add a blank line 
    f_Out << "\n";

    std::vector<mafTimeStamp> kframes1;
    std::vector<mafTimeStamp> kframes2;
    m_ForceLeft->GetTimeStamps(kframes1);
    m_ForceRight->GetTimeStamps(kframes2);
    std::vector<mafTimeStamp> kframes = MergeTimeStamps(kframes1,kframes2);
    int size = kframes.size();

    // Get first data to set right input position
    m_ForceLeft->SetTimeStamp(kframes.at(0));
    m_ForceLeft->Update();
    m_ForceLeft->GetOutput()->GetVTKData()->Update();
    m_MomentLeft->SetTimeStamp(kframes.at(0));
    m_MomentLeft->Update();
    m_MomentLeft->GetOutput()->GetVTKData()->Update();
    m_ForceRight->SetTimeStamp(kframes.at(0));
    m_ForceRight->Update();
    m_ForceRight->GetOutput()->GetVTKData()->Update();
    m_MomentRight->SetTimeStamp(kframes.at(0));
    m_MomentRight->Update();
    m_MomentRight->GetOutput()->GetVTKData()->Update();

    double* init_posCOPL = m_ForceLeft->GetOutput()->GetVTKData()->GetPoint(0);
    double* init_posCOPR = m_ForceRight->GetOutput()->GetVTKData()->GetPoint(0);
    double* init_posML = m_MomentLeft->GetOutput()->GetVTKData()->GetPoint(1);
    double* init_posMR = m_MomentRight->GetOutput()->GetVTKData()->GetPoint(1);

    medGRFVector* vFL = new medGRFVector;
    medGRFVector* vFR = new medGRFVector;
    medGRFVector* vML = new medGRFVector;
    medGRFVector* vMR = new medGRFVector;
    for (int i=0;i<6;i++)
    {
      vFL->m_Bounds[i]=0;
      vFR->m_Bounds[i]=0;
      vML->m_Bounds[i]=0;
      vMR->m_Bounds[i]=0;
    }
    vFL->m_SwitchX = vFL->m_SwitchY = vFL->m_SwitchZ = 0;
    vFR->m_SwitchX = vFR->m_SwitchY = vFR->m_SwitchZ = 0;
    vML->m_SwitchX = vML->m_SwitchY = vML->m_SwitchZ = 0;
    vMR->m_SwitchX = vMR->m_SwitchY = vMR->m_SwitchZ = 0;

    //Add times and values; time is always the first row
    for (int i=0;i<size;i++)
    {
      double time = kframes.at(i);

      // ---------------------
      // FORCE LEFT ----------
      // ---------------------

      // Get bounds
      mafOBB obb;
      m_ForceLeft->GetOutput()->GetBounds(obb,time);
      vFL->m_Bounds[0] = (obb.m_Bounds)[0];
      vFL->m_Bounds[1] = (obb.m_Bounds)[1];
      vFL->m_Bounds[2] = (obb.m_Bounds)[2];
      vFL->m_Bounds[3] = (obb.m_Bounds)[3];
      vFL->m_Bounds[4] = (obb.m_Bounds)[4];
      vFL->m_Bounds[5] = (obb.m_Bounds)[5];

      // Switch direction if case
      int ind1 = ((vFL->m_SwitchX==0) ? 1 : 0);
      int ind2 = ((vFL->m_SwitchY==0) ? 2 : 3);
      int ind3 = ((vFL->m_SwitchZ==0) ? 4 : 5);
      CheckVectorToSwitch(i,ind1,ind2,ind3,vFL,init_posCOPL,m_ForceLeft,0,time);
      ind1 = ((vFL->m_SwitchX==0) ? 1 : 0);
      ind2 = ((vFL->m_SwitchY==0) ? 2 : 3);
      ind3 = ((vFL->m_SwitchZ==0) ? 4 : 5);

      // TIME
      f_Out << time << ",";
            
      // COP L
      double copL[3];
      copL[0] = vFL->m_Bounds[ind1];
      copL[1] = vFL->m_Bounds[ind2];
      copL[2] = vFL->m_Bounds[ind3];
      f_Out << copL[0] << "," << copL[1] << "," << copL[2] << ",";
      
      // REF L
      f_Out << 0 << "," << 0 << "," << 0 << ",";

      // FORCE L
      ind1 = ((vFL->m_SwitchX==0) ? 0 : 1);
      ind2 = ((vFL->m_SwitchY==0) ? 3 : 2);
      ind3 = ((vFL->m_SwitchZ==0) ? 5 : 4);
      double fL[3];
      fL[0] =  vFL->m_Bounds[ind1];
      fL[1] =  vFL->m_Bounds[ind2];
      fL[2] =  vFL->m_Bounds[ind3];
      f_Out << fL[0]-copL[0] << "," << fL[1]-copL[1] << "," << fL[2]-copL[2] << ",";

      // ---------------------
      // MOMENT LEFT ---------
      // ---------------------

      // Get bounds
      m_MomentLeft->GetOutput()->GetBounds(obb,time);
      vML->m_Bounds[0] = (obb.m_Bounds)[0];
      vML->m_Bounds[1] = (obb.m_Bounds)[1];
      vML->m_Bounds[2] = (obb.m_Bounds)[2];
      vML->m_Bounds[3] = (obb.m_Bounds)[3];
      vML->m_Bounds[4] = (obb.m_Bounds)[4];
      vML->m_Bounds[5] = (obb.m_Bounds)[5];

      // Switch direction if case
      ind1 = ((vML->m_SwitchX==0) ? 1 : 0);
      ind2 = ((vML->m_SwitchY==0) ? 3 : 2);
      ind3 = ((vML->m_SwitchZ==0) ? 5 : 4);
      CheckVectorToSwitch(i,ind1,ind2,ind3,vML,init_posML,m_MomentLeft,1,time);
      ind1 = ((vML->m_SwitchX==0) ? 1 : 0);
      ind2 = ((vML->m_SwitchY==0) ? 3 : 2);
      ind3 = ((vML->m_SwitchZ==0) ? 5 : 4);

      // MOMENT L
      double mL[3];
      mL[0] = vML->m_Bounds[ind1];
      mL[1] = vML->m_Bounds[ind2];
      mL[2] = vML->m_Bounds[ind3];
      f_Out << mL[0]-copL[0] << "," << mL[1]-copL[1] << "," << mL[2]-copL[2] << ",";

      // ---------------------
      // FORCE RIGHT ---------
      // ---------------------

      // Get bounds
      m_ForceRight->GetOutput()->GetBounds(obb,time);
      vFR->m_Bounds[0] = (obb.m_Bounds)[0];
      vFR->m_Bounds[1] = (obb.m_Bounds)[1];
      vFR->m_Bounds[2] = (obb.m_Bounds)[2];
      vFR->m_Bounds[3] = (obb.m_Bounds)[3];
      vFR->m_Bounds[4] = (obb.m_Bounds)[4];
      vFR->m_Bounds[5] = (obb.m_Bounds)[5];

      // Switch direction if case
      ind1 = ((vFR->m_SwitchX==0) ? 0 : 1);
      ind2 = ((vFR->m_SwitchY==0) ? 3 : 2);
      ind3 = ((vFR->m_SwitchZ==0) ? 4 : 5);
      CheckVectorToSwitch(i,ind1,ind2,ind3,vFR,init_posCOPR,m_ForceRight,0,time);
      ind1 = ((vFR->m_SwitchX==0) ? 0 : 1);
      ind2 = ((vFR->m_SwitchY==0) ? 3 : 2);
      ind3 = ((vFR->m_SwitchZ==0) ? 4 : 5);

      // COP R
      double copR[3];
      copR[0] = vFR->m_Bounds[ind1];
      copR[1] = vFR->m_Bounds[ind2];
      copR[2] = vFR->m_Bounds[ind3];
      f_Out << copR[0] << "," << copR[1] << "," << copR[2] << ",";
      
      // REF R
      f_Out << 0 << "," << 0 << "," << 0 << ",";
      
      // FORCE R
      ind1 = (vFR->m_SwitchX==0) ? 1 : 0;
      ind2 = (vFR->m_SwitchY==0) ? 2 : 3;
      ind3 = (vFR->m_SwitchZ==0) ? 5 : 4;
      double fR[3];
      fR[0] = vFR->m_Bounds[ind1];
      fR[1] = vFR->m_Bounds[ind2];
      fR[2] = vFR->m_Bounds[ind3];
      f_Out << fR[0]-copR[0] << "," << fR[1]-copR[1] << "," << fR[2]-copR[2] << ",";

      // ---------------------
      // MOMENT RIGHT --------
      // ---------------------
      
      // Get bounds
      m_MomentRight->GetOutput()->GetBounds(obb,time);
      vMR->m_Bounds[0] = (obb.m_Bounds)[0];
      vMR->m_Bounds[1] = (obb.m_Bounds)[1];
      vMR->m_Bounds[2] = (obb.m_Bounds)[2];
      vMR->m_Bounds[3] = (obb.m_Bounds)[3];
      vMR->m_Bounds[4] = (obb.m_Bounds)[4];
      vMR->m_Bounds[5] = (obb.m_Bounds)[5];

      // Switch direction if case
      ind1 = ((vMR->m_SwitchX==0) ? 0 : 1);
      ind2 = ((vMR->m_SwitchY==0) ? 3 : 2);
      ind3 = ((vMR->m_SwitchZ==0) ? 5 : 4);
      CheckVectorToSwitch(i,ind1,ind2,ind3,vMR,init_posMR,m_MomentRight,1,time);
      ind1 = ((vMR->m_SwitchX==0) ? 0 : 1);
      ind2 = ((vMR->m_SwitchY==0) ? 3 : 2);
      ind3 = ((vMR->m_SwitchZ==0) ? 5 : 4);

      // MOMENT R
      double mR[3];
      mR[0] = vMR->m_Bounds[ind1];
      mR[1] = vMR->m_Bounds[ind2];
      mR[2] = vMR->m_Bounds[ind3];
      f_Out << mR[0]-copR[0] << "," << mR[1]-copR[1] << "," << mR[2]-copR[2] << "\n";

      if (!m_TestMode)
      {
        mafEventMacro(mafEvent(this,PROGRESSBAR_SET_VALUE,(long)(((double) i)/((double) size)*100.)));
      }
    }

    delete vFL;
    delete vFR;
    delete vML;
    delete vMR;
    
    f_Out.close();
  }  

  if (!m_TestMode)
  {
    wxMessageBox("THE FAST METHOD ENDED SUCCESSFULLY!\nPlease import the output and compare the result with the original!","GRF EXPORTER FAST",wxOK);
    mafEventMacro(mafEvent(this,PROGRESSBAR_HIDE));
    wxSetCursor(wxCursor(wxCURSOR_DEFAULT));
    cppDEL(wait);
  }
}
//----------------------------------------------------------------------------
void medOpExporterGRFWS::CheckVectorToSwitch(int frame, int coor1_ID, int coor2_ID, int coor3_ID, medGRFVector* v, double* original_pos, mafVMEVector* vVME, int pointID, mafTimeStamp time)
//----------------------------------------------------------------------------
{
  /* Check right input position:
  This method guesses frame by frame for each vector if the vector is going to change direction (by passing through its origin).
  In this case we must flip the vector coordinates.
  The vector direction are under control by cheking if its bounds, after decreasing, suddenly increase. This could be a signal the vector is changing
  direction and in this case the origina VTK data is loaded and the original position is controlled.
  THIS METHOD HAS BEEN IMPLEMENTED TO REDUCE SIGNIFICATIVELY THE TIME CONSUMPTION, BUT IT'S NOT SAFE, even if it has been tested to work 100% of the times.
  */

  // Flip check for corners position
  if (frame==0)
  {
    if (abs(v->m_Bounds[coor1_ID]-original_pos[0])>0.05)
    {
      v->m_SwitchX = !v->m_SwitchX;
    }
    if (abs(v->m_Bounds[coor2_ID]-original_pos[1])>0.05)
    {
      v->m_SwitchY = !v->m_SwitchY;
    }
    if (abs(v->m_Bounds[coor3_ID]-original_pos[2])>0.05)
    {
      v->m_SwitchZ = !v->m_SwitchZ;
    }
  }
  // Calculate distance
  else if (frame==1)
  {
    double distance[3];
    distance[0] = abs((v->m_Bounds[coor1_ID])-(v->m_Bounds[!coor1_ID]));
    distance[1] = abs((v->m_Bounds[coor2_ID])-(v->m_Bounds[5-coor2_ID]));
    distance[2] = abs((v->m_Bounds[coor3_ID])-(v->m_Bounds[9-coor3_ID]));
    for (int j=0;j<3;j++)
    {
      v->m_OldDistance[j] = distance[j];
    }
    // Must check ever!
    if (true)
    {
      vVME->SetTimeStamp(time);
      vVME->Update();
      vVME->GetOutput()->GetVTKData()->Update();
      double* pos = vVME->GetOutput()->GetVTKData()->GetPoint(pointID);
      CheckVectorToSwitch(0,coor1_ID,coor2_ID,coor3_ID,v,pos);
    }
  }
  // Calculate distance and growing flag
  else if (frame==2)
  {    
    double distance[3];
    distance[0] = abs((v->m_Bounds[coor1_ID])-(v->m_Bounds[!coor1_ID]));
    distance[1] = abs((v->m_Bounds[coor2_ID])-(v->m_Bounds[5-coor2_ID]));
    distance[2] = abs((v->m_Bounds[coor3_ID])-(v->m_Bounds[9-coor3_ID]));
    for (int j=0;j<3;j++)
    {
      if (distance[j]<=v->m_OldDistance[j]) 
      {
        v->m_IsDecreasing[j] = 1;
      }
      else
      {
        v->m_IsDecreasing[j] = 0;
      }
      v->m_OldDistance[j] = distance[j];
      v->m_WasDecreasing[j] = v->m_IsDecreasing[j];
    }
    // Must check ever!
    if (true)
    {
      vVME->SetTimeStamp(time);
      vVME->Update();
      vVME->GetOutput()->GetVTKData()->Update();
      double* pos = vVME->GetOutput()->GetVTKData()->GetPoint(pointID);
      CheckVectorToSwitch(0,coor1_ID,coor2_ID,coor3_ID,v,pos);
    }
  }
  // Check if the vector is changing direction. In case load VTK data and perform the flip check
  else
  {
    double distance[3];
    distance[0] = abs((v->m_Bounds[coor1_ID])-(v->m_Bounds[!coor1_ID]));
    distance[1] = abs((v->m_Bounds[coor2_ID])-(v->m_Bounds[5-coor2_ID]));
    distance[2] = abs((v->m_Bounds[coor3_ID])-(v->m_Bounds[9-coor3_ID]));
    bool switched = 0;
    for (int j=0;j<3;j++)
    {
      if (distance[j]<=v->m_OldDistance[j])
      {
        v->m_IsDecreasing[j] = 1;
      }
      else
      {
        v->m_IsDecreasing[j] = 0;
      }
      if (!switched && (distance[j]<m_Treshold && v->m_OldDistance[j]<m_Treshold) && (v->m_WasDecreasing[j] && !v->m_IsDecreasing[j]) )
      {
        vVME->SetTimeStamp(time);
        vVME->Update();
        vVME->GetOutput()->GetVTKData()->Update();
        double* pos = vVME->GetOutput()->GetVTKData()->GetPoint(pointID);
        CheckVectorToSwitch(0,coor1_ID,coor2_ID,coor3_ID,v,pos);
        switched = 1;
      }
      v->m_OldDistance[j] = distance[j];
      v->m_WasDecreasing[j] = v->m_IsDecreasing[j];
    }
  }
}
//----------------------------------------------------------------------------
std::vector<mafTimeStamp> medOpExporterGRFWS::MergeTimeStamps(std::vector<mafTimeStamp> kframes1,std::vector<mafTimeStamp> kframes2)   
//----------------------------------------------------------------------------
{
  std::vector<mafTimeStamp> kframes;

  int i = 0;
  int j = 0;

  if (kframes1.size()==0 && kframes2.size()==0)
  {
    kframes.resize(0);
    return kframes;
  }

  do
  {
    if (kframes1.at(i)<=kframes2.at(j))
    {
      kframes.push_back(kframes1.at(i));
      if (kframes1.at(i)==kframes2.at(j))
      {
        j++;
      }
      i++;
    }
    else
    {
      kframes.push_back(kframes2.at(j));
      j++;
    }
  } while (i<(kframes1.size()) && j<(kframes2.size()));

  return kframes;
}
//----------------------------------------------------------------------------
void medOpExporterGRFWS::SetFileName(const char *file_name)   
//----------------------------------------------------------------------------
{
  m_File = file_name;

  m_File_temp1 = m_File + "_tmp1";
  m_File_temp2 = m_File + "_tmp2";
  m_File_temp3 = m_File + "_tmp3";
  m_File_temp4 = m_File + "_tmp4";
}
//----------------------------------------------------------------------------
void medOpExporterGRFWS::RemoveTempFiles()   
//----------------------------------------------------------------------------
{
  remove(m_File_temp1);
  remove(m_File_temp2);
  remove(m_File_temp3);
  remove(m_File_temp4);
}