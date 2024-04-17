-module(comp).

-export([comp/1, comp_proc/2, comp_proc/3, decomp/1, decomp/2]).
-export([comp_workers_loop/3, decomp_workers_loop/3]).

-define(DEFAULT_CHUNK_SIZE, 1024*1024).
-define(DEFAULT_PROCS, 3).

%%% File Compression

comp(File) ->
    comp_proc(File,?DEFAULT_CHUNK_SIZE,?DEFAULT_PROCS).

comp_proc(File,Procs) ->
    comp_proc(File,?DEFAULT_CHUNK_SIZE,Procs).

comp_proc(File, Chunk_Size, Procs) ->
    case file_service:start_file_reader(File, Chunk_Size) of
        {ok, Reader} ->
            case archive:start_archive_writer(File++".ch") of
                {ok, Writer} ->
                    Parent = self(),
                    init_comp_proc(Procs, [], Reader, Writer, Parent),
                    wait_procs(Procs);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.

init_comp_proc(0, _List, _Reader, _Writer, _Parent) ->
    ok;
init_comp_proc(Procs, List, Reader, Writer, Parent) ->
    Newpid = spawn(?MODULE, comp_workers_loop, [Reader, Writer, Parent]),
    init_comp_proc(Procs-1, [Newpid|List], Reader, Writer, Parent).

wait_procs(0) ->
    done;
wait_procs(Procs) ->
    receive
        done -> wait_procs(Procs-1)
    end.

comp_workers_loop(Reader, Writer, Parent) ->
    Reader ! {get_chunk, self()},
    receive
        {chunk, Num, Offset, Data} ->
            Comp_Data = compress:compress(Data),
            Writer ! {add_chunk, Num, Offset, Comp_Data},
            comp_workers_loop(Reader, Writer, Parent);
        eof ->
            Parent ! done;
        {error, Reason} ->
            Parent ! {error, Reason}
    end.

%%% File Decompression

decomp(Archive) ->
    decomp(Archive, string:replace(Archive, ".ch", "", trailing)).

decomp(Archive, Output_File) ->
    case archive:start_archive_reader(Archive) of
        {ok, Reader} ->
            case file_service:start_file_writer(Output_File) of
                {ok, Writer} ->
                    Parent = self(),
                    init_decomp_proc(Reader, Writer, Parent),
                    wait_decomp();
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.

init_decomp_proc(Reader, Writer, Parent) ->
    spawn(?MODULE, decomp_workers_loop, [Reader, Writer, Parent]).

wait_decomp() ->
    receive
        done -> ok
    end.

decomp_workers_loop(Reader, Writer, Parent) ->
    Reader ! {get_chunk, self()},
    receive
        {chunk, _Num, Offset, Comp_Data} ->
            Data = compress:decompress(Comp_Data),
            Writer ! {write_chunk, Offset, Data},
            decomp_workers_loop(Reader, Writer, Parent);
        eof ->
            Parent ! done;
        {error, Reason} ->
            Parent ! {error, Reason}
    end.
