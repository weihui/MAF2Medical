/*=========================================================================

 Program: MAF2Medical
 Module: medWizardBlock
 Authors: Gianluigi Crimi
 
 Copyright (c) B3C
 All rights reserved. See Copyright.txt or
 http://www.scsitaly.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __medWizardBlock_H__
#define __medWizardBlock_H__

//----------------------------------------------------------------------------
// includes :
//----------------------------------------------------------------------------
#include "medCommonDefines.h"
#include "mafEvent.h"

//----------------------------------------------------------------------------
// forward reference
//----------------------------------------------------------------------------

/**
  Class Name: medWizardBlock.
  Class for the base wizard block, each block on a wizard must extend this block
*/
class MED_COMMON_EXPORT medWizardBlock 
{
public:

  /** Default constructor   
      Requires the name of the block 
      "START","END", "ABORT" are reserved. */
  medWizardBlock(const char *name);

  /** Default destructor */
  virtual ~medWizardBlock();

  
  /** Get the name of the block */
  wxString GetName();

  /** Return the name of the Block witch will be executed after this */
  virtual wxString GetNextBlock();

  /** Abort the execution of the block */
  virtual void Abort();

  /** Return true if the user has aborted the operation */
  int Success();

  /** Starts the execution of the block */
  virtual void ExcutionBegin();

  /** Ends the execution of the block */
  virtual void ExcutionEnd();

  /** Returns the name of the operation required by this block 
      Return an empty string if no operation is required */
  virtual wxString GetRequiredOperation();

  /** Set the selected VME, this function must be called before execution begin*/
  void SetSelectedVME(mafNode *node);

  /** Sets The event listener */
  void SetListener(mafObserver *Listener);

  /** Called to clean up memory*/
  virtual void Delete(){delete this;};

  /** Get a Label containing a description of the the current step*/
  mafString GetDescriptionLabel();

  /** Set a Label containing a description of the the current step*/
  void SetDescriptionLabel(const char *label);

  /** Set a Label containing a description of the the current step*/
  void SetOnAbortNextBlock(const char *label);

protected:

  wxString m_Name;
  wxString m_BlockType;
  wxString m_AbortBlock;
  mafNode	*m_SelectedVME; ///< Pointer to the current selected node.
  mafNode *m_InputVME; ///< The vme selected on operation start.
  int m_Success;
  int m_Running;
  mafObserver    *m_Listener;
  mafString m_DescriptionLabel;

private:
    
  
};
#endif
