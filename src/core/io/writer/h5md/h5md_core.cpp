/*
  Copyright (C) 2010,2011,2012,2013,2014,2015,2016 The ESPResSo project
  Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2009,2010
    Max-Planck-Institute for Polymer Research, Theory Group

  This file is part of ESPResSo.

  ESPResSo is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  ESPResSo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "h5md_core.hpp"


namespace Writer {
namespace H5md {

static void backup_file(const std::string &from, const std::string& to)
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    if (this_node == 0)
    {
        /*
         * If the file itself *and* a backup file exists something must
         * went wrong before.
        */
        boost::filesystem::path pfrom(from), pto(to);
        try
        {
            boost::filesystem::copy_file(pfrom, pto,
                boost::filesystem::copy_option::fail_if_exists);
        } catch(const boost::filesystem::filesystem_error& e)
        {
            throw left_backupfile();
        }
    }
}

static std::vector<hsize_t>
create_dims(hsize_t dim, hsize_t size, hsize_t chunk_size=0)
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    if (dim == 3)
        return std::vector<hsize_t>{chunk_size, size, dim};
    if (dim == 2)
        return std::vector<hsize_t>{size, dim};
    if (dim == 1)
        return std::vector<hsize_t>{size};
    else
        throw std::runtime_error("H5MD Error: datastets with this dimension are not implemented\n");
}

//Correct Chunking is important for the IO performance!
std::vector<hsize_t>
File::create_chunk_dims(hsize_t dim, hsize_t size, hsize_t chunk_size)
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
	if (dim == 3)
	    return std::vector<hsize_t>{chunk_size, size, dim};
	if (dim == 2)
	    return std::vector<hsize_t>{chunk_size, size};
	if (dim == 1)
	    return std::vector<hsize_t>{size};
	else
	    throw std::runtime_error("H5MD Error: datastets with this dimension are not implemented\n");

}
static std::vector<hsize_t> create_maxdims(hsize_t dim)
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    if (dim == 3)
        return std::vector<hsize_t>{H5S_UNLIMITED, H5S_UNLIMITED, H5S_UNLIMITED};
    if (dim == 2)
        return std::vector<hsize_t>{H5S_UNLIMITED, H5S_UNLIMITED};
    if (dim == 1)
        return std::vector<hsize_t>{H5S_UNLIMITED};
    else
        throw std::runtime_error("H5MD Error: datastets with this dimension are not implemented\n");
}




/* Initialize the file related variables after parameters have been set. */
void File::InitFile()
{
#ifdef H5MD_DEBUG
    H5Eset_auto(H5E_DEFAULT, (H5E_auto_t) H5Eprint, stderr);
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    //use a seperate mpi communicator if we want to write out ordered data. This is in order to avoid  blocking by collective functions
    if(m_write_ordered == true)
        MPI_Comm_split(MPI_COMM_WORLD, this_node, 0, &m_hdf5_comm); 
    else
        m_hdf5_comm=MPI_COMM_WORLD;
    if(m_write_ordered == true and this_node !=0)
        return;
    if(n_part <= 0) {
        throw std::runtime_error("Please first set up particles before initializing the H5md object."); //this is important since n_part is used for chunking
    }
    boost::filesystem::path script_path(m_scriptname);
    m_absolute_script_path = boost::filesystem::canonical(script_path);
    init_filestructure();
    bool fileexists = check_file_exists(m_filename);
    /* Perform a barrier synchronization. Otherwise one process might already
     * create the file while another still checks for its existence. */
    if(m_write_ordered == false)
        MPI_Barrier(m_hdf5_comm);
    if (fileexists) {
        if (check_for_H5MD_structure(m_filename)) {
            /*
             * If the file exists and has a valid H5MD structure, lets create a
             * backup of it.  This has the advantage, that the new file can
             * just be deleted if the simulation crashes at some point and we
             * still have a valid trajectory, we can start from.
            */
            m_backup_filename = m_filename + ".bak";
            if (this_node == 0) backup_file(m_filename, m_backup_filename);
            load_file(m_filename);
        } else {
            throw incompatible_h5mdfile();
        }
    } else {
        create_new_file(m_filename);
    }
}



void File::init_filestructure()
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    group_names = {
        "particles",
        "particles/atoms",
        "particles/atoms/box",
        "particles/atoms/mass",
        "particles/atoms/id",
        "particles/atoms/type",
        "particles/atoms/position",
        "particles/atoms/velocity",
        "particles/atoms/force",
        "particles/atoms/image",
        "parameters",
        "parameters/files"
    };
    h5xx::datatype type_double = h5xx::datatype(H5T_NATIVE_DOUBLE);
    h5xx::datatype type_int = h5xx::datatype(H5T_NATIVE_INT);
    hsize_t npart = static_cast<hsize_t>(n_part);
    dataset_descriptors = {
            //path, dim, type
        { "particles/atoms/box/edges"     , 1, type_double },
        { "particles/atoms/mass/value"    , 2, type_double },
        { "particles/atoms/id/value"      , 2, type_int },
        { "particles/atoms/id/time"       , 1, type_double },
        { "particles/atoms/id/step"       , 1, type_int },
        { "particles/atoms/type/value"    , 2, type_int },
        { "particles/atoms/position/value", 3, type_double },
        { "particles/atoms/velocity/value", 3, type_double },
        { "particles/atoms/force/value"   , 3, type_double },
        { "particles/atoms/image/value"   , 3, type_int },
    };
}

void File::create_datasets(bool only_load)
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    for (const auto& descr: dataset_descriptors) {
        const std::string& path = descr.path;

        if (only_load) {
            datasets[path] = h5xx::dataset(m_h5md_file, path);
        } else {
            int creation_size_dataset = 0; //creation size of all datasets is 0. Make sure to call ExtendDataset before writing to dataset
            int chunk_size = 1;
            if(descr.dim>1){
                //we deal now with a particle based property, change chunk. Important for IO performance!
                chunk_size=n_part;
            }
            auto dims = create_dims(descr.dim, creation_size_dataset);
            auto chunk_dims = create_chunk_dims(descr.dim, chunk_size, 1);
            auto maxdims = create_maxdims(descr.dim);
            auto storage = h5xx::policy::storage::chunked(chunk_dims).set(
                    h5xx::policy::storage::fill_value(-10));
            auto dataspace = h5xx::dataspace(dims, maxdims);
            hid_t lcpl_id = H5Pcreate(H5P_LINK_CREATE);
            H5Pset_create_intermediate_group(lcpl_id, 1);
            datasets[path] = h5xx::dataset(m_h5md_file, path, descr.type,dataspace,storage, lcpl_id, H5P_DEFAULT);
        }
    }
    if(! only_load)
        create_links_for_time_and_step_datasets();
}

void File::create_links_for_time_and_step_datasets(){
	H5Eset_auto(H5E_DEFAULT, (H5E_auto_t) H5Eprint, stderr);
	std::string path_time="particles/atoms/id/time";
	std::string path_step="particles/atoms/id/step";
	H5Lcreate_hard( m_h5md_file.hid(), path_time.c_str(), m_h5md_file.hid(), "particles/atoms/image/time", H5P_DEFAULT, H5P_DEFAULT );
	H5Lcreate_hard( m_h5md_file.hid(), path_step.c_str(), m_h5md_file.hid(), "particles/atoms/image/step", H5P_DEFAULT, H5P_DEFAULT );
	H5Lcreate_hard( m_h5md_file.hid(), path_time.c_str(), m_h5md_file.hid(), "particles/atoms/force/time", H5P_DEFAULT, H5P_DEFAULT );
	H5Lcreate_hard( m_h5md_file.hid(), path_step.c_str(), m_h5md_file.hid(), "particles/atoms/force/step", H5P_DEFAULT, H5P_DEFAULT );
	H5Lcreate_hard( m_h5md_file.hid(), path_time.c_str(), m_h5md_file.hid(), "particles/atoms/velocity/time", H5P_DEFAULT, H5P_DEFAULT );
	H5Lcreate_hard( m_h5md_file.hid(), path_step.c_str(), m_h5md_file.hid(), "particles/atoms/velocity/step", H5P_DEFAULT, H5P_DEFAULT );
	H5Lcreate_hard( m_h5md_file.hid(), path_time.c_str(), m_h5md_file.hid(), "particles/atoms/position/time", H5P_DEFAULT, H5P_DEFAULT );
	H5Lcreate_hard( m_h5md_file.hid(), path_step.c_str(), m_h5md_file.hid(), "particles/atoms/position/step", H5P_DEFAULT, H5P_DEFAULT );
	H5Lcreate_hard( m_h5md_file.hid(), path_time.c_str(), m_h5md_file.hid(), "particles/atoms/type/time", H5P_DEFAULT, H5P_DEFAULT );
	H5Lcreate_hard( m_h5md_file.hid(), path_step.c_str(), m_h5md_file.hid(), "particles/atoms/type/step", H5P_DEFAULT, H5P_DEFAULT );
	H5Lcreate_hard( m_h5md_file.hid(), path_time.c_str(), m_h5md_file.hid(), "particles/atoms/mass/time", H5P_DEFAULT, H5P_DEFAULT );
	H5Lcreate_hard( m_h5md_file.hid(), path_step.c_str(), m_h5md_file.hid(), "particles/atoms/mass/step", H5P_DEFAULT, H5P_DEFAULT );
}

void File::load_file(const std::string& filename)
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    m_h5md_file = h5xx::file(filename, m_hdf5_comm, MPI_INFO_NULL,
                             h5xx::file::out);
#ifdef H5MD_DEBUG
    std::cout << "Finished opening the h5 file on node " << this_node << std::endl;
#endif
    bool only_load = true;
    create_datasets(only_load);
}

void File::create_new_file(const std::string &filename)
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    this->WriteScript(filename);
    /* Create a new h5xx file object. */
    m_h5md_file = h5xx::file(filename, m_hdf5_comm, MPI_INFO_NULL,
                             h5xx::file::out);

    bool only_load = false;
    create_datasets(only_load);
    std::vector<double> boxvec = {box_l[0], box_l[1], box_l[2]};
    auto group = h5xx::group(m_h5md_file,  "particles/atoms/box");
    h5xx::write_attribute(group, "dimension", 3);
    h5xx::write_attribute(group, "boundary", "periodic");
    std::string path_edges = "particles/atoms/box/edges";
    int extent_edges = 3; //for three entries for cuboid box box_l_x, box_l_y, box_l_z
    ExtendDataset(path_edges, extent_edges);
    h5xx::write_dataset(datasets[path_edges], boxvec);
}


void File::Close()
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    if (this_node == 0) boost::filesystem::remove(m_backup_filename);
}


void File::fill_arrays_for_h5md_write_with_particle_property(int particle_index, int_array_3d& id, int_array_3d& typ, double_array_3d& mass, double_array_3d& pos, int_array_3d& image, double_array_3d& vel, double_array_3d& f, Particle* current_particle,bool write_typ,bool write_mass,bool write_pos, bool write_vel, bool write_force ){
#ifdef H5MD_DEBUG
    /* Turn on hdf5 error messages */
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
	        id[0][particle_index][0] = current_particle->p.identity;
            if (write_typ)
                typ[0][particle_index][0] = current_particle->p.type;
            if (write_mass)
                mass[0][particle_index][0] = current_particle->p.mass;
            /* store folded particle positions. */
            if (write_pos)
            {
                pos[0][particle_index][0] = current_particle->r.p[0];
                pos[0][particle_index][1] = current_particle->r.p[1];
                pos[0][particle_index][2] = current_particle->r.p[2];
                image[0][particle_index][0] = current_particle->l.i[0];
                image[0][particle_index][1] = current_particle->l.i[1];
                image[0][particle_index][2] = current_particle->l.i[2];
            }
            if (write_vel)
            {
                vel[0][particle_index][0] = current_particle->m.v[0] / time_step;
                vel[0][particle_index][1] = current_particle->m.v[1] / time_step;
                vel[0][particle_index][2] = current_particle->m.v[2] / time_step;
            }
            if (write_force)
            {
                /* Scale the stored force with m/(0.5*dt**2.0) to get a real 
                 * world force. */
                double fac = current_particle->p.mass / (0.5 * time_step * time_step);
                f[0][particle_index][0] = current_particle->f.f[0] * fac;
                f[0][particle_index][1] = current_particle->f.f[1] * fac;
                f[0][particle_index][2] = current_particle->f.f[2] * fac;
            }

}

void File::Write(int write_dat)
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    int num_particles_to_be_written;
    if(m_write_ordered == true && this_node == 0)
        num_particles_to_be_written=n_part;
    else if(m_write_ordered == true && this_node !=0)
        return ;
    else if (m_write_ordered == false)
        num_particles_to_be_written=cells_get_n_particles();
    
    bool write_typ = write_dat & W_TYPE;
    bool write_pos = write_dat & W_POS;
    bool write_vel = write_dat & W_V;
    bool write_force = write_dat & W_F;
    bool write_mass = write_dat & W_MASS;

    double_array_3d pos(boost::extents[1][num_particles_to_be_written][3]);
    double_array_3d vel(boost::extents[1][num_particles_to_be_written][3]);
    double_array_3d f(boost::extents[1][num_particles_to_be_written][3]);
    int_array_3d image(boost::extents[1][num_particles_to_be_written][3]);
    int_array_3d id(boost::extents[1][num_particles_to_be_written][1]);
    int_array_3d typ(boost::extents[1][num_particles_to_be_written][1]);
    double_array_3d mass(boost::extents[1][num_particles_to_be_written][1]);
    double_array_3d time(boost::extents[1][1][1]);
    time[0][0][0]=sim_time;
    int_array_3d step(boost::extents[1][1][1]);
    step[0][0][0]=(int)std::round(sim_time/time_step);

    if (m_write_ordered == true) {
        if( this_node == 0 ){ 
            //loop over all particles
            for(int particle_index=0;particle_index<n_part;particle_index++){
                Particle current_particle;
	            get_particle_data(particle_index, &current_particle); //this function only works when run with one process
                fill_arrays_for_h5md_write_with_particle_property(particle_index, id, typ, mass, pos, image, vel, f, &current_particle,  write_typ, write_mass, write_pos, write_vel, write_force );
	            free_particle(&current_particle);
            }
        }

    }else{
        /* Get the number of particles on all other nodes. */

        /* loop over all local cells. */
        Cell *local_cell;
        int particle_index = 0;
        for (int cell_id = 0; cell_id < local_cells.n; ++cell_id)
        {
            local_cell = local_cells.cell[cell_id];
            for (int local_part_id = 0;
                 local_part_id < local_cell->n; ++local_part_id)
            {
                auto & current_particle = local_cell->part[local_part_id];
                fill_arrays_for_h5md_write_with_particle_property(particle_index, id, typ, mass, pos, image, vel, f, &current_particle, write_typ, write_mass, write_pos, write_vel, write_force );
                particle_index++;
            }
        }
    }

    WriteDataset(id, "particles/atoms/id/value");
    WriteDataset(time, "particles/atoms/id/time");
    WriteDataset(step, "particles/atoms/id/step");

    if (write_typ)
    {
        WriteDataset(typ, "particles/atoms/type/value");
    }
    if (write_mass)
    {
        WriteDataset(mass, "particles/atoms/mass/value");
    }
    if (write_pos)
    {
        WriteDataset(pos, "particles/atoms/position/value");
        WriteDataset(image, "particles/atoms/image/value");
    }
    if (write_vel)
    {
        WriteDataset(vel, "particles/atoms/velocity/value");
    }
    if (write_force)
    {
        WriteDataset(f, "particles/atoms/force/value");
    }

}


void File::ExtendDataset(std::string path, int extent=1){
    /* Until now the h5xx does not support dataset extending, so we
       have to use the lower level hdf5 library functions. */
    auto& dataset = datasets[path];
    /* Get the current dimensions of the dataspace. */
    hid_t ds = H5Dget_space(dataset.hid());
    hsize_t rank = H5Sget_simple_extent_ndims(ds);
    hsize_t dims[rank], maxdims[rank];
    H5Sget_simple_extent_dims(ds, dims, maxdims);
    H5Sclose(ds);
    /* Extend the dataset for another timestep (extent=1) */
    dims[0] += extent;
    
    // Extend the dataset for more particles if the particle number increased
    if(rank>1){
        /* Get the number of particles on all other nodes. */
        int n_part = std::max((int) dims[1],max_seen_particle+1); //take into account previous dimension, if we append to an already existing dataset
        if (n_part > m_max_n_part) {
        	m_max_n_part = n_part; 
        }
        dims[1] = m_max_n_part;
    }
    H5Dset_extent(dataset.hid(), dims); //extend all dims is collective
}

/* data is assumed to be three dimensional */
template <typename T>
void File::WriteDataset(T &data, const std::string& path)
{
#ifdef H5MD_DEBUG
    /* Turn on hdf5 error messages */
    H5Eset_auto(H5E_DEFAULT, (H5E_auto_t) H5Eprint, stderr);
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
    std::cout << "Dataset: " << path << std::endl;
#endif
    auto& dataset = datasets[path];
    ExtendDataset(path);

    int num_particles_to_be_written = data.shape()[1];
    int prefix = 0;
    //calculate prefix for write of the current process
    MPI_Exscan(&num_particles_to_be_written, &prefix, 1, MPI_INT, MPI_SUM, m_hdf5_comm);
    hid_t ds = H5Dget_space(dataset.hid());
    hsize_t rank = H5Sget_simple_extent_ndims(ds);
    /* Get the current dimensions of the dataspace. */
    hsize_t dims[rank], maxdims[rank];
    H5Sget_simple_extent_dims(ds, dims, maxdims);
    H5Sclose(ds);
    /* Select the region in the dataspace. */
    hsize_t offset[rank];
    hsize_t count[rank];
    hid_t ds_new;
    ds = H5Dget_space(dataset.hid());
    offset[0]= dims[0]-1; //offset for time (-1 since hdf5 counts from 0 onwards)
    count [0] = 1; //count for time
    if(rank>1){
        //particle based offset and count
        offset[1]= static_cast<hsize_t>(prefix);
        count [1] = static_cast<hsize_t>(num_particles_to_be_written);
     }
    if(rank>2){
        //coordinate based offset and count
        offset[2]= 0;
        count [2] = data.shape()[2];
    }
            
    H5Sselect_hyperslab(ds, H5S_SELECT_SET, offset, NULL, count, NULL);
    /* Create a temporary dataspace. */
    ds_new = H5Screate_simple(rank, count, maxdims);
    /* Finally write the data to the dataset. */
    H5Dwrite(dataset.hid(),
             dataset.get_type(),
             ds_new,
             ds, H5P_DEFAULT,
             data.origin());
    H5Sclose(ds_new);
    H5Sclose(ds);
}


void File::WriteScript(std::string const &filename)
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    /* First get the number of lines of the script. */
    hsize_t dims[1] = {1};
    std::string tmp;
    std::ifstream scriptfile(m_absolute_script_path.string());
    /* Read the whole script into a buffer. */
    scriptfile.seekg(0, std::ios::end);
    auto filelen = scriptfile.tellg();
    scriptfile.seekg(0);
    std::vector<char> buffer;
    buffer.reserve(filelen);
    buffer.assign(std::istreambuf_iterator<char>(scriptfile),
                  std::istreambuf_iterator<char>());

    hid_t filetype, dtype, space, dset, file_id;
    file_id = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    dtype = H5Tcopy(H5T_C_S1);
    H5Tset_size(dtype, filelen * sizeof(char));

    space = H5Screate_simple(1, dims, NULL);
    /* Create the dataset. */
    hid_t link_crt_plist = H5Pcreate(H5P_LINK_CREATE);
    H5Pset_create_intermediate_group(link_crt_plist, true);	// Set flag for intermediate group creation
    dset = H5Dcreate(file_id, "parameters/files/script", dtype, space, link_crt_plist, H5P_DEFAULT, H5P_DEFAULT);
    /* Write data from buffer to dataset. */
    if (this_node == 0)
        H5Dwrite(dset, dtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
    /* Clean up. */
    H5Dclose(dset);
    H5Sclose(space);
    H5Tclose(dtype);
    H5Fclose(file_id);
}


bool File::check_for_H5MD_structure(std::string const &filename)
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
    h5xx::file h5mdfile(filename, h5xx::file::in);

    for (const auto& gnam: group_names)
        if (!h5xx::exists_group(h5mdfile, gnam))
            return false;

    for (const auto& ddesc: dataset_descriptors)
        if (!h5xx::exists_dataset(h5mdfile, ddesc.path))
            return false;

    return true;
}

/* Constructor */
File::File ()
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
}

/* Desctructor */
File::~File()
{
#ifdef H5MD_DEBUG
    std::cout << "Called " << __func__ << " on node " << this_node << std::endl;
#endif
}
} /* namespace H5md */
} /* namespace Writer */
