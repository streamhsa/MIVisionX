#include "image_source_evaluator.h"

void ImageSourceEvaluator::set_size_evaluation_policy(MaxSizeEvaluationPolicy arg) 
{
    _width_max.set_policy (arg); 
    _height_max.set_policy (arg);
}

size_t ImageSourceEvaluator::max_width()
{
    return _width_max.get_max();
}

size_t ImageSourceEvaluator::max_height()
{
    return _height_max.get_max();
}  

ImageSourceEvaluatorStatus 
ImageSourceEvaluator::create(StorageType storage_type, DecoderType decoder_type, const std::string &path)
{
    ImageSourceEvaluatorStatus status = ImageSourceEvaluatorStatus::OK;

    // Can initialize it to any decoder types if needed
    

    _header_buff.resize(COMPRESSED_SIZE);
    switch(decoder_type)
    {
        case DecoderType::TURBO_JPEG:
        {
            _decoder = std::make_shared<TJDecoder>();
        }
        break;
        default:
        {
            THROW("Unsupported decoder type "+ TOSTR(decoder_type))
        }
    }
    switch(storage_type)
    {
        case StorageType::FILE_SYSTEM:
        {
            auto reader_desc = FileSourceReaderConfig(path);
            _reader = create_reader(&reader_desc);
        }
        break;
        default:
        {
            THROW("Unsupported storage type "+ TOSTR(storage_type));
        }
    }
    find_max_dimension();
    return status;
}

void 
ImageSourceEvaluator::find_max_dimension()
{
    _reader->reset();

    while( _reader->count() ) 
    {
        if( (_reader->open()) == 0 )
            continue;

        auto actual_read_size = _reader->read(_header_buff.data(), COMPRESSED_SIZE);
        _reader->close();
        
        int width, height, jpeg_sub_samp;
        if(_decoder->decode_info(_header_buff.data(), actual_read_size, &width, &height, &jpeg_sub_samp ) != Decoder::Status::OK)
        {
            WRN("Could not decode the header of the: "+ _reader->id())
            continue;
        }
        
        if(width <= 0 || height <=0)
            continue;

        _width_max.process_sample(width);
        _height_max.process_sample(height);

    }
    // return the reader read pointer to the begining of the resource
    _reader->reset();
    _reader->reset();
}

void 
ImageSourceEvaluator::FindMaxSize::process_sample(unsigned val)
{
    if(_policy == MaxSizeEvaluationPolicy::MAXIMUM_FOUND_SIZE)
    {
        _max = (val > _max) ? val : _max;
    }
    if(_policy == MaxSizeEvaluationPolicy::MOST_FREQUENT_SIZE)
    {
        auto it = _hist.find(val);
        size_t count = 1;
        if( it != _hist.end())
        {
            it->second =+ 1;
            count = it->second;
        } else {
            _hist.insert(std::make_pair(val, 1));
        }

        unsigned new_count = count;
        if(new_count > _max_count)
        {
            _max = val;
            _max_count = new_count;
        }        
    }
}