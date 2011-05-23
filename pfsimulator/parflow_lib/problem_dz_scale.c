/*BHEADER**********************************************************************

  Copyright (c) 1995-2009, Lawrence Livermore National Security,
  LLC. Produced at the Lawrence Livermore National Laboratory. Written
  by the Parflow Team (see the CONTRIBUTORS file)
  <parflow@lists.llnl.gov> CODE-OCEC-08-103. All rights reserved.

  This file is part of Parflow. For details, see
  http://www.llnl.gov/casc/parflow

  Please read the COPYRIGHT file or Our Notice and the LICENSE file
  for the GNU Lesser General Public License.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License (as published
  by the Free Software Foundation) version 2.1 dated February 1999.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the IMPLIED WARRANTY OF
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the terms
  and conditions of the GNU General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA
**********************************************************************EHEADER*/

#include "parflow.h"

/*--------------------------------------------------------------------------
 * Structures
 *--------------------------------------------------------------------------*/

typedef struct
{
   NameArray   regions;
   int         type;
   int         variable_dz;
   void       *data;
} PublicXtra;

typedef void InstanceXtra;

typedef struct
{
   int         num_regions;
   int        *region_indices;
   double     *values;
} Type0;                       /* constant regions */

typedef struct
{
   char       *filename;
   Vector     *values;
} Type1;                       /* from PFB */

typedef struct
{
    int         num_dz;
    double     *values;
} Type2;                       /* from list in tcl array */

/*--------------------------------------------------------------------------
 * dZ Scaling values
 *--------------------------------------------------------------------------*/
void dzScale (ProblemData *problem_data, Vector *dz_mult )
{
   PFModule       *this_module   = ThisPFModule;
   PublicXtra     *public_xtra   = (PublicXtra *)PFModulePublicXtra(this_module);

   Grid           *grid          = VectorGrid(dz_mult);

   SubgridArray   *subgrids      = GridSubgrids(grid);
   Subgrid        *subgrid;
   Subvector      *ps_sub;
   Subvector      *dz_sub;
   Subvector      *val_sub;
    
   VectorUpdateCommHandle       *handle;
    
    
   double         *data;
   double         *dz_dat;
   double         *val_dat;

   int             ix, iy, iz;
   int             nx, ny, nz;
   int             r;
   int             is, i, j, k, ips, ipicv;
   int             ii;


   /*-----------------------------------------------------------------------
    * dz Scale
    *-----------------------------------------------------------------------*/

   InitVectorAll(dz_mult, 1.0);

   if (public_xtra -> variable_dz) {
   switch((public_xtra -> type))
   {

   // constant regions
   case 0:
   {

      Type0   *dummy0;
      int      num_regions;
      int     *region_indices;
      double  *values;

      GrGeomSolid  *gr_solid;
      double        value;
      int           ir;

      dummy0 = (Type0 *)(public_xtra -> data);

      num_regions    = (dummy0 -> num_regions);
      region_indices = (dummy0 -> region_indices);
      values         = (dummy0 -> values);

      for (ir = 0; ir < num_regions; ir++)
      {

         printf( "problem_dz_scale -- setting by regions (region %d) \n", is );

	 gr_solid = ProblemDataGrSolid(problem_data, region_indices[ir]);
	 value    = values[ir];

	 ForSubgridI(is, subgrids)
	 {

            printf( "problem_dz_scale -- setting for subgrids (subgrid %d) \n", is );

            subgrid = SubgridArraySubgrid(subgrids, is);
            ps_sub  = VectorSubvector(dz_mult, is);
	    
	    ix = SubgridIX(subgrid);
	    iy = SubgridIY(subgrid);
	    iz = SubgridIZ(subgrid);
	    
	    nx = SubgridNX(subgrid);
	    ny = SubgridNY(subgrid);
	    nz = SubgridNZ(subgrid);
	    
	    /* RDF: assume resolution is the same in all 3 directions */
	    r = SubgridRX(subgrid);
	    
	    data = SubvectorData(ps_sub);
	    GrGeomInLoop(i, j, k, gr_solid, r, ix, iy, iz, nx, ny, nz,
            {                
	       ips = SubvectorEltIndex(ps_sub, i, j, k);
	       data[ips] = value;
              //  printf("dz %d %d %d %f %f \n",i,j, k, data[ips],value);

	    });
	 }
      }

      break;
   }

   // from PFB
   case 1:
   {
      
      Type1   *dummy1;
      dummy1 = (Type1 *)(public_xtra -> data);

      char        *filename  = dummy1 -> filename;
      Vector      *values    = dummy1 -> values;
      GrGeomSolid *gr_domain = ProblemDataGrDomain(problem_data);

      values     = NewVectorType( grid, 1, 1, vector_cell_centered );
      ReadPFBinary( filename, values );   

      ForSubgridI(is, subgrids)
      {

         printf( "problem_dz_scale -- setting from PFB (subgrid %d) \n", is );

         subgrid = SubgridArraySubgrid(subgrids, is);
         dz_sub  = VectorSubvector(dz_mult, is);
         val_sub = VectorSubvector(values,is);
         dz_dat  = SubvectorData(dz_sub);
         val_dat = SubvectorData(val_sub);

         ix = SubgridIX(subgrid);
         iy = SubgridIY(subgrid);
         iz = SubgridIZ(subgrid);

         nx = SubgridNX(subgrid);
         ny = SubgridNY(subgrid);
         nz = SubgridNZ(subgrid);

         /* RDF: assume resolution is the same in all 3 directions */
         r = SubgridRX(subgrid);

         GrGeomInLoop(i, j, k, gr_domain, r, ix, iy, iz, nx, ny, nz,
         {

            ips   = SubvectorEltIndex(dz_sub,  i, j, k);
            ipicv = SubvectorEltIndex(val_sub, i, j, k);
            dz_dat[ips] = val_dat[ipicv];
            // printf("dz %d %d %d %d %d %f %f \n",i,j, k,ips, ipicv, dz_dat[ips],val_dat[ipicv]);


         });
      }        /* End subgrid loop */

      break;
  
   }

           // from list of values, could be function in the future
       case 2:
       {
           Type2    *dummy2;
           dummy2 = (Type2 *)(public_xtra -> data);

           int      num_dz;
           double  *values;
           
           num_dz    = (dummy2 -> num_dz);
           values    = (dummy2 -> values);
           
          /* for (ii=0; ii < num_dz; ii++) {
               printf("dz %d %f \n",ii, values[ii]);
           } */
           
      	   //GrGeomSolid  *gr_domain;     
        
           GrGeomSolid *gr_domain = ProblemDataGrDomain(problem_data);
           

           ForSubgridI(is, subgrids)
           {
               
               printf( "problem_dz_scale -- setting from list of dz values \n", is );
               
               subgrid = SubgridArraySubgrid(subgrids, is);
               dz_sub  = VectorSubvector(dz_mult, is);
               dz_dat  = SubvectorData(dz_sub);
               
               ix = SubgridIX(subgrid);
               iy = SubgridIY(subgrid);
               iz = SubgridIZ(subgrid);
               
               nx = SubgridNX(subgrid);
               ny = SubgridNY(subgrid);
               nz = SubgridNZ(subgrid);
               
               /* RDF: assume resolution is the same in all 3 directions */
               r = SubgridRX(subgrid);
               
               GrGeomInLoop(i, j, k, gr_domain, r, ix, iy, iz, nx, ny, nz,
                            {
                                
                                ips   = SubvectorEltIndex(dz_sub,  i, j, k);
                                dz_dat[ips] = values[k];
                            //    printf("dz %d %d %d %f %f \n",i,j, k, dz_dat[ips],values[k]);

                                
                            });
           }        /* End subgrid loop */
           
           break;
           
       }
           
           
   }
   }
    handle = InitVectorUpdate(dz_mult, VectorUpdateAll);
    FinalizeVectorUpdate(handle);
   }


/*--------------------------------------------------------------------------
 * dzScaleInitInstanceXtra
 *--------------------------------------------------------------------------*/

PFModule  *dzScaleInitInstanceXtra()
{
   PFModule      *this_module   = ThisPFModule;
   InstanceXtra  *instance_xtra;

#if 0
   if ( PFModuleInstanceXtra(this_module) == NULL )
      instance_xtra = ctalloc(InstanceXtra, 1);
   else
      instance_xtra = (InstanceXtra *)PFModuleInstanceXtra(this_module);
#endif
   instance_xtra = NULL;

   PFModuleInstanceXtra(this_module) = instance_xtra;

   return this_module;
}

/*-------------------------------------------------------------------------
 * dzScaleFreeInstanceXtra
 *-------------------------------------------------------------------------*/

void  dzScaleFreeInstanceXtra()
{
   PFModule      *this_module   = ThisPFModule;
   InstanceXtra  *instance_xtra = (InstanceXtra *)PFModuleInstanceXtra(this_module);

   if (instance_xtra)
   {
      tfree(instance_xtra);
   }

}

/*--------------------------------------------------------------------------
 * dzScaleNewPublicXtra
 *--------------------------------------------------------------------------*/

PFModule   *dzScaleNewPublicXtra()
{
   PFModule      *this_module   = ThisPFModule;
   PublicXtra    *public_xtra;

   Type0         *dummy0;
   Type1         *dummy1;
   Type2         *dummy2;

   int            num_regions, ir; 
   char *switch_name;
   int  *switch_value;
   char *region;
   char *nzListValues;
   char  key[IDB_MAX_KEY_LEN];
   char *name;
   NameArray switch_na;

   public_xtra = ctalloc(PublicXtra, 1);

   /* @RMM added switch for grid dz multipliers */
   /* RMM set dz multipliers (default=False) */
   // printf("flag 1: pre name set \n");
   name                   = "Solver.Nonlinear.VariableDz";
   switch_na              = NA_NewNameArray("False True");
   switch_name            = GetStringDefault(name, "False");
   switch_value           = NA_NameToIndex(switch_na, switch_name);

   if(switch_value < 0)
   {
        InputError("Error: invalid value <%s> for key <%s>\n",
                   switch_name, key );
   }
    
   public_xtra -> variable_dz = switch_value;
   if (public_xtra -> variable_dz == 1) { 
      
      name                   = "dzScale.Type"; 
      switch_na              = NA_NewNameArray("Constant PFBFile nzList");
      switch_name            = GetString(name);
      public_xtra -> type    = NA_NameToIndex(switch_na,switch_name);

       name                   = "dzScale.GeomNames";
       switch_name            = GetString(name);
       public_xtra -> regions = NA_NewNameArray(switch_name);
       num_regions            = NA_Sizeof(public_xtra -> regions);
       
       
      // switch for dzScale.Type
      // Constant = 0; PFBFile = 1;
      switch( (public_xtra -> type) )
      {

         // Set as constant by regions
	 case 0:
	 {
         

            printf( "problem_dz_scale -- setting dz_mult by regions (type=Constant) \n" );

	    dummy0 = ctalloc(Type0, 1);
 
            (dummy0 -> num_regions)    = num_regions;
	    (dummy0 -> region_indices) = ctalloc(int,    num_regions);
	    (dummy0 -> values)         = ctalloc(double, num_regions);
	    
	    for (ir = 0; ir < num_regions; ir++)
	    {
	       region = NA_IndexToName(public_xtra -> regions, ir);

	       dummy0 -> region_indices[ir] = 
		  NA_NameToIndex(GlobalsGeomNames, region);

	       sprintf(key, "Geom.%s.dzScale.Value", region);
	       dummy0 -> values[ir] = GetDouble(key);
	    }
	    
	    (public_xtra -> data) = (void *) dummy0;
	    
	    break;
	 }

         // Read from PFB file	
         case 1:
         {
  
            printf( "problem_dz_scale -- setting dz_mult from file (type=PFBFile) \n" );

            dummy1 = ctalloc(Type1, 1);

            sprintf(key, "Geom.%s.dzScale.FileName", "domain");
            dummy1 -> filename = GetString(key);

            public_xtra -> data = (void *) dummy1;

            break;

         }
              
              //Set from tcl list; map to grid
          case 2:
          {
              
              printf( "problem_dz_scale -- setting dz_mult tcl list \n" );
              
              dummy2 = ctalloc(Type2, 1);
              
              name                        = "dzScale.nzListNumber";
            //  switch_name                 = GetString(name);
            //  nzListValues                = NA_NewNameArray(switch_name);
              
              (dummy2 -> num_dz)          = GetDouble(name);
              // printf("Ncell %d \n",  dummy2 -> num_dz);

              (dummy2 -> values)          = ctalloc(double, (dummy2 -> num_dz)); 
              for (ir=0; ir < (dummy2 -> num_dz); ir++) {
                  sprintf(key, "Cell.%d.dzScale.Value", ir);
                  dummy2 -> values[ir] = GetDouble(key);
                 // printf("cell %d %s %f \n",ir, key, dummy2 -> values[ir]);
                 // printf("dz1 %d %f \n",ir, dummy2 -> values[ir]);

              }
              (public_xtra -> data) = (void *) dummy2;
              
              break;
              
          }
              
	 default:
	 {
	    InputError("Error: invalid type <%s> for key <%s>\n",
		       switch_name, key);
	 }

      }
    }
   
   
   PFModulePublicXtra(this_module) = public_xtra;
   return this_module;
}

/*--------------------------------------------------------------------------
 * dzScaleFreePublicXtra
 *--------------------------------------------------------------------------*/

void  dzScaleFreePublicXtra()
{
   PFModule    *this_module   = ThisPFModule;
   PublicXtra  *public_xtra   = (PublicXtra *)PFModulePublicXtra(this_module);

   Type0       *dummy0;
   Type1       *dummy1;
   Type2       *dummy2;

    
   if (public_xtra -> variable_dz) {

      NA_FreeNameArray(public_xtra -> regions);

      switch((public_xtra -> type))
      {
         case 0:
         {
            dummy0 = (Type0 *)(public_xtra -> data);
	    tfree(dummy0 -> region_indices);
	    tfree(dummy0 -> values);
            tfree(dummy0);
            break;
         }

         case 1:
         {
            dummy1 = (Type1 *)(public_xtra -> data);
            tfree(dummy1);
            break;
         } 

          case 2:
          {
              dummy2 = (Type2 *)(public_xtra -> data);
              tfree(dummy2);
              break;
          } 
              
      }

   }else {
        
      tfree(public_xtra);

   }
}

/*--------------------------------------------------------------------------
 * dzScaletorageSizeOfTempData
 *--------------------------------------------------------------------------*/

int  dzScaleSizeOfTempData()
{
   return 0;
}
