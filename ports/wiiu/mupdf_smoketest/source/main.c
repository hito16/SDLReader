#include <coreinit/thread.h>
#include <coreinit/time.h>

#include <whb/log.h>
#include <whb/log_cafe.h>
#include <whb/log_console.h>
#include <whb/proc.h>

#include <mupdf/fitz.h>

#include "sample_pdf.h"

// scripts/run-in-cemu.sh stops the emulator as soon as it sees this line.
#define RUN_DONE_MARKER "CEMU_RUN_DONE"

static OSTime t0;

static int
elapsed_ms(void)
{
   return (int)OSTicksToMilliseconds(OSGetTime() - t0);
}

static int
run_mupdf(void)
{
   fz_context *ctx;
   fz_stream *stm = NULL;
   fz_document *doc = NULL;
   fz_pixmap *pix = NULL;
   int ok = 0;

   ctx = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
   if (!ctx) {
      WHBLogPrintf("FAIL: fz_new_context");
      return 0;
   }
   WHBLogPrintf("MuPDF %s context created", FZ_VERSION);

   fz_var(stm);
   fz_var(doc);
   fz_var(pix);
   fz_try(ctx) {
      fz_register_document_handlers(ctx);

      stm = fz_open_memory(ctx, sample_pdf, sample_pdf_size);
      doc = fz_open_document_with_stream(ctx, "application/pdf", stm);
      WHBLogPrintf("opened embedded PDF (%u bytes), pages=%d [%d ms]",
                   (unsigned)sample_pdf_size, fz_count_pages(ctx, doc), elapsed_ms());

      pix = fz_new_pixmap_from_page_number(ctx, doc, 0, fz_scale(1, 1), fz_device_rgb(ctx), 0);
      WHBLogPrintf("rendered page 1: %dx%d n=%d [%d ms]", pix->w, pix->h, pix->n, elapsed_ms());

      // A blank page would mean fonts/rasterising silently failed.
      unsigned int inked = 0, sum = 0;
      for (int y = 0; y < pix->h; y++) {
         unsigned char *row = pix->samples + y * pix->stride;
         for (int x = 0; x < pix->w * pix->n; x += pix->n) {
            if (row[x] < 128 || row[x + 1] < 128 || row[x + 2] < 128)
               inked++;
            sum = sum * 31 + row[x] + row[x + 1] + row[x + 2];
         }
      }
      WHBLogPrintf("dark pixels=%u checksum=%08x", inked, sum);
      ok = inked > 0;
   }
   fz_always(ctx) {
      fz_drop_pixmap(ctx, pix);
      fz_drop_document(ctx, doc);
      fz_drop_stream(ctx, stm);
   }
   fz_catch(ctx) {
      WHBLogPrintf("FAIL: %s", fz_caught_message(ctx));
      ok = 0;
   }

   fz_drop_context(ctx);
   return ok;
}

int
main(int argc, char **argv)
{
   WHBProcInit();
   WHBLogCafeInit();    // OSReport -> Cemu log.txt (captured by cemu-run)
   WHBLogConsoleInit(); // draws the log on the TV/GamePad

   t0 = OSGetTime();
   WHBLogPrintf("mupdf_smoketest starting");
   WHBLogConsoleDraw();

   WHBLogPrintf(run_mupdf() ? "RESULT: PASS" : "RESULT: FAIL");
   WHBLogPrintf(RUN_DONE_MARKER);
   WHBLogConsoleDraw();

   while (WHBProcIsRunning()) {
      WHBLogConsoleDraw();
      OSSleepTicks(OSMillisecondsToTicks(100));
   }

   WHBLogConsoleFree();
   WHBLogCafeDeinit();
   WHBProcShutdown();
   return 0;
}
