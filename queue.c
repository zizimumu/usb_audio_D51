

unsigned char playback_index[8];
unsigned int playback_wr = 0;
unsigned int playback_rd = 0;

void init_playbackQueue(void)
{
	playback_wr = playback_rd = 0;
}
void add_playbackQueue(unsigned char value)
{
	
	playback_index[playback_wr] = value;
	playback_wr++;

	if(playback_wr >= sizeof(playback_index))
		playback_wr = 0;

}

int queue_playbackEmpty()
{
	if(playback_wr == playback_rd)
		return 1;
	else
		return 0;
}

unsigned char get_playbackQueue(void)
{
	unsigned char val;

	
	val = playback_index[playback_rd];
	playback_rd++;
	
	if(playback_rd >= sizeof(playback_index))
		playback_rd = 0;

	return val;

}




unsigned char capture_index[16];
unsigned int cputure_wr = 0;
unsigned int cputure_rd = 0;

void init_queue(void)
{
	cputure_wr = cputure_rd = 0;
}
void add_queue(unsigned char value)
{
	
	capture_index[cputure_wr] = value;
	cputure_wr++;

	if(cputure_wr >= sizeof(capture_index))
		cputure_wr = 0;

}

int queue_empty()
{
	if(cputure_wr == cputure_rd)
		return 1;
	else
		return 0;
}


// read point is always in front of write point
unsigned char get_queue(void)
{
	unsigned char val;

	
	val = capture_index[cputure_rd];
	cputure_rd++;
	
	if(cputure_rd >= sizeof(capture_index))
		cputure_rd = 0;

	return val;

}
