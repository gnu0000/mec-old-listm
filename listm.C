/*
 *
 * list.c
 * Friday, 1/9/1998.
 * Craig Fitz
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutype.h>
#include <gnufile.h>
#include <gnustr.h>
#include <gnumisc.h>
#include <gnumem.h>
#include <gnuwin.h>
#include <gnukbd.h>

typedef struct _cel
   {
   PSZ psz;            // Matter#/task/subtask  string
   PSZ pszDesc;        // Matter#/task/subtask  Description
   struct _cel *next;  // Next Matter#/task/subtask
   struct _cel *child; // pclH->Matter# or Matter#->Task or Task->subtask
   } CEL;
typedef CEL *PCEL;

PCEL pclH;                          // data tree head;
PGW  ppgw [4];                      // popup windows

UINT uLINES       = 10;             // default window size in lines
UINT uWINSIZES[4] = {33, 7, 7, 27}; // window horizontal sizes
UINT uActive      = 0;              // active window

/***************************************************************************/
/***************************************************************************/

PCEL GetCellPtr (PGW pgw, UINT uIndex)
   {
   PCEL pclP, pcl;
   UINT i;

   pclP = (PCEL) pgw->pUser1;
   if (!pclP || uIndex > pgw->uItemCount)
      return NULL;
   for (i=0, pcl = pclP->child; pcl && i<uIndex; i++)
      pcl = pcl->next;
   return pcl;
   }

void SetItemCount (PGW pgw)
   {
   PCEL pclP, pcl;
   UINT i;

   if (!(pclP = (PCEL) pgw->pUser1))
      {
      pgw->uItemCount = 0;
      }
   else
      {
      for (i=0, pcl = pclP->child; pcl; i++, pcl = pcl->next)
         ;
      pgw->uItemCount = i;
      }
   GnuSelectLine (pgw, (pgw->uItemCount ? 0 : 0xFFFF), TRUE);
   }

void UpdateInfoWin (void)
   {
   UINT i, uX;
   PCEL pcl;

   GnuPaintNChar (ppgw[3], 0, 0, 0, 0, ' ', ppgw[3]->uClientXSize);
   for (i=0; i<3; i++)
      {
      GnuPaintNChar (ppgw[3], i+2, 4, 0, 0, ' ', ppgw[3]->uClientXSize-4);

      if (pcl = GetCellPtr (ppgw[i], ppgw[i]->uSelection))
         {
         uX = (i==0 ? 1 : (i==1 ? 10 : 15));
         GnuPaint (ppgw[3], 0, uX, 0, 1, pcl->psz);
         GnuPaint (ppgw[3], i+2, 4, 0, 1, pcl->pszDesc);
         }
      }
   }

void PaintChildren (UINT uActive)
   {
   UINT i;

   for (i=uActive; i<2; i++)
      {
      ppgw[i+1]->pUser1 = GetCellPtr (ppgw[i], ppgw[i]->uSelection);
      SetItemCount (ppgw[i+1]);
      GnuPaintWin (ppgw[i+1], 0xFFFF);
      }
   UpdateInfoWin ();
   }

UINT CCONV pPaint (PGW pgw, UINT uIndex, UINT uLine)
   {
   UINT uLevel, uAtt;
   PCEL pcl;

   if (!pgw->pUser1 || uIndex >= pgw->uItemCount)
      return 0;

   uLevel = (USHORT)(ULONG)pgw->pUser2;
   uAtt   = (pgw->uSelection == uIndex ? (uLevel == uActive ? 3 : 1) : 0);
   pcl    = GetCellPtr (pgw, uIndex);

   GnuPaintNChar (pgw, uLine, 0, 0, uAtt, ' ', pgw->uClientXSize);
   GnuPaint (pgw, uLine, 1, 0, uAtt, pcl->psz);
   if (uLevel == 0)
      GnuPaint (pgw, uLine, 10, 0, uAtt, pcl->pszDesc);
   return 1;
   }

/***************************************************************************/

void CreatePopup ()
   {
   UINT i, uPos = 1;

   for (i=0; i<4; i++)  
      {
      ppgw[i] = GnuCreateWin2 (1, uPos, uLINES, uWINSIZES[i], pPaint); // matter#
      uPos += uWINSIZES[i] - 1;
      ppgw[i]->pUser2 = (PVOID)(ULONG)i;
      }
   for (i=1; i<4; i++)  
      {
      GnuPaintNChar (ppgw[i], -1, -1, 0, 0, 'Ë', 1);
      GnuPaintNChar (ppgw[i], BottomOf(ppgw[i])-2, -1, 0, 0, 'Ê', 1);
      }
   ppgw[0]->pUser1 = pclH;
   SetItemCount (ppgw[0]);
   uActive = 0;
   GnuPaintWin (ppgw[0], 0xFFFF);
   PaintChildren (uActive);
   GnuPaintBig (ppgw[3], 2, 1, 4, 3, 0, 0, "M:\nT:\nS:");
   GnuPaint (ppgw[3], BottomOf(ppgw[3])-4, 1, 0, 0, "<\x18\x19\x1A\x1B> Move Selection");
   GnuPaint (ppgw[3], BottomOf(ppgw[3])-3, 1, 0, 0, "<C> Copy to clipboard");
   }

void DestroyPopup (void)
   {
   UINT i;

   for (i=4; i; i--)
      GnuDestroyWin (ppgw[i-1]);
   }

UINT CopyToClipboard (void)
   {
   CHAR sz[128];
   PCEL pcl1, pcl2, pcl3;

   pcl1 = GetCellPtr (ppgw[0], ppgw[0]->uSelection);
   pcl2 = GetCellPtr (ppgw[1], ppgw[1]->uSelection);
   pcl3 = GetCellPtr (ppgw[2], ppgw[2]->uSelection);

   sprintf (sz, "echo %s\t%s\t%s >CLIP:", 
               (pcl1->psz ? pcl1->psz : "   "),
               (pcl2->psz ? pcl2->psz : "   "),
               (pcl3->psz ? pcl3->psz : "   "));
   system (sz);
   return GNUBeep (2);
   }

void ControlPopup (void)
   {
   UINT c, uOldActive;

   while (TRUE)
      {
      uOldActive = uActive;
      switch (c = KeyGet (TRUE))
         {
         case K_RIGHT:
            uActive = (uActive + 1) % 3;
            GnuPaintWin (ppgw[uOldActive], ppgw[uOldActive]->uSelection);
            GnuPaintWin (ppgw[uActive],    ppgw[uActive]->uSelection);
            break;

         case K_LEFT:
            uActive = (uActive + 2) % 3;
            GnuPaintWin (ppgw[uOldActive], ppgw[uOldActive]->uSelection);
            GnuPaintWin (ppgw[uActive],    ppgw[uActive]->uSelection);
            break;

         case K_ESC:
            return;

         case 'C':
            CopyToClipboard ();
            break;

         default:
            if (GnuDoListKeys (ppgw[uActive], c))
               PaintChildren (uActive);
            else
               GNUBeep (0);
         }
      }
   }

void ShowPopup (PCEL pcl)
   {
   CreatePopup ();
   ControlPopup ();
   DestroyPopup ();
   }

/***************************************************************************/
/***************************************************************************/

PCEL NewCell (PSZ psz, PSZ pszDesc)
   {
   PCEL pcl;

   pcl = calloc (1, sizeof (CEL));
   pcl->psz     = strdup (psz);
   pcl->pszDesc = strdup (pszDesc);

   return pcl;
   }

void AddCell (PCEL pclNew, UINT uLevel) //m:1 t:2 s:3
   {
   UINT i;
   PCEL pcl;

   pcl = pclH;
   for (i=0; i<uLevel; i++)
      {
      if (!pcl->child)
         {
         if (i+1 < uLevel)
            Error ("No parent object for %s (line %ld)", pclNew->psz, FilGetLine ());
         pcl->child = pclNew;
         return;
         }
      for (pcl = pcl->child; pcl->next; pcl = pcl->next)
         ;
      }
   pcl->next = pclNew;
   }

PCEL ReadFile (PSZ pszFile)
   {
   PCEL pcl;
   FILE *fp;
   CHAR sz[256];
   PPSZ ppsz;
   UINT uCols, c;

   if (!(fp = fopen (pszFile, "rt")))
      Error ("Could not open file %s", pszFile);

   pclH = NewCell ("000", "Matter Number Tree");

   while (FilReadLine (fp, sz, ";", sizeof (sz)) != (INT) -1)
      {
      if (StrBlankLine (sz))
         continue;

      ppsz = StrMakePPSZ (sz, ",", TRUE, TRUE, &uCols);
      c    = toupper(*ppsz[0]);
      pcl  = NewCell (ppsz[1], ppsz[2]);

      AddCell (pcl, (c == 'S' ? 3 : (c == 'T' ? 2 : 1)));
      MemFreePPSZ (ppsz, uCols);
      }
   fclose (fp);
   return pclH;
   }

/***************************************************************************/
/***************************************************************************/

//void Dump (PCEL pclStart, UINT uLevel)
//   {
//   PCEL pcl;
//   UINT i;
//
//   for (pcl = pclStart; pcl; pcl = pcl->next)
//      {
//      for (i=0; i<uLevel; i++)
//         printf ("   ");
//      printf ("%s  -  %s\n", pcl->psz, pcl->pszDesc);
//      Dump (pcl->child, uLevel+1);
//      }
//   }

int main (int argc, char *argv[])
   {
   PCEL pcl;

   if (argc > 1)
      uLINES = Range (3, atoi(argv[1]), 25);

   pcl = ReadFile ("Matter.csv");
   ShowPopup (pcl);
   return 0;
   }
