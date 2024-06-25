/* DreamShell ##version##

   module.h - Games app module
   Copyright (C) 2024 Maniac Vera

*/

#include <ds.h>
#include "app_utils.h"
#include "app_module.h"
#include "app_definition.h"
#include <kos/md5.h>
#include <drivers/rtc.h>
#include <isoldr.h>
#include <tsunami/tsudefinition.h>
#include <tsunami/tsunamiutils.h>
#include <tsunami/font.h>
#include <tsunami/color.h>
#include <tsunami/drawable.h>
#include <tsunami/texture.h>
#include <tsunami/trigger.h>
#include <tsunami/vector.h>
#include <tsunami/anims/expxymover.h>
#include <tsunami/anims/logxymover.h>
#include <tsunami/triggers/death.h>
#include <tsunami/dsmenu.h>

#define IN_CACHE_GAMES

#define DEBUG_MENU_GAMES

DEFAULT_MODULE_EXPORTS(app_games);

kthread_t *exit_menu_thread;

static struct
{
	App_t *app;
   bool have_args;
   DSMenu *dsmenu_ptr;
   Scene *scene_ptr;
   int image_type;
   int sector_size;
   int current_dev;
   isoldr_info_t *isoldr;
   uint32 addr;
   uint8 md5[16];
   uint8 boot_sector[2048];

   bool exit;
   bool exit_app;
   struct timeval old_time;
   struct timeval new_time;
   int menu_type;
   int pages;
   int current_page;
   int previous_page;
   int game_count;
   int menu_cursel;
   int games_array_count;
   char default_dir[20];
   char games_path[NAME_MAX];
   char covers_path[50];
   char item_value_selected[NAME_MAX];
   GameItemStruct *games_array;   
   Texture *item_selector_texture;
   Animation *item_selector_animation;
   Banner *item_selector;
   Font *menu_font;
   Trigger *exit_trigger_list[MAX_SIZE_ITEMS];
   Animation *exit_animation_list[MAX_SIZE_ITEMS];
   Animation *img_button_animation[MAX_SIZE_ITEMS];
   ItemMenu *img_button[MAX_SIZE_ITEMS];
   Label *title;    
   MenuOptionStruct menu_option;   
} self;


static void SetTitle(const char *text)
{
   char titleText[101];
   memset(titleText, 0, sizeof(titleText));

   // static struct mallinfo mi;
   // mi = mallinfo(); 
   // snprintf(titleText, sizeof(titleText), "%s - %i", text, mi.fordblks);

   if (strlen(text) > sizeof(titleText)) {
      strncpy(titleText, text, sizeof(titleText) - 1);
   }
   else {
      strcpy(titleText, text);
   }

   if (self.title != NULL) { 
      TSU_LabelSetText(self.title, titleText);
   }
   else {
      static Vector vector = {10, 30, ML_ITEM, 1};
      static Color color = {1, 1.0f, 1.0f, 0.1f};
      self.title = TSU_LabelCreate(self.menu_font, titleText, 24, false, true);

      TSU_LabelSetTranslate(self.title, &vector);
      TSU_LabelSetTint(self.title, &color);      
      TSU_MenuSubAddLabel(self.dsmenu_ptr, self.title);
   }      

   StopCDDATrack();
   // if (GUI_WidgetGetState(self.cdda)) {
      char filepath[NAME_MAX];
      size_t track_size = GetCDDATrackFilename(5, self.games_path, text, filepath);

      if (track_size) {
         do {
            track_size = GetCDDATrackFilename((random() % 15) + 4, self.games_path, text, filepath);
         } while(track_size == 0);
         PlayCDDATrack(filepath, 3);
      }
   // }
}

static void SetMenuType(int menu_type)
{
   self.menu_type = menu_type;

   switch (menu_type)
   {
      case MT_IMAGE_TEXT_64_5X2:
         {
               self.menu_option.max_page_size = 10;
               self.menu_option.max_columns = 2;
               self.menu_option.size_items_column = self.menu_option.max_page_size / self.menu_option.max_columns;
               self.menu_option.init_position_x = 20;
               self.menu_option.init_position_y = 5;
               self.menu_option.padding_x = 320;
               self.menu_option.padding_y = 12;
               self.menu_option.image_size = 64.0f;
         }
         break;

      case MT_IMAGE_128_4X3: 
         {
               self.menu_option.max_page_size = 12;
               self.menu_option.max_columns = 4;
               self.menu_option.size_items_column = self.menu_option.max_page_size / self.menu_option.max_columns;
               self.menu_option.init_position_x = 55;
               self.menu_option.init_position_y = -50;
               self.menu_option.padding_x = 163;
               self.menu_option.padding_y = 12;
               self.menu_option.image_size = 128.0f;
         }
         break;

      case MT_IMAGE_256_3X2:
         {
               self.menu_option.max_page_size = 6;
               self.menu_option.max_columns = 3;
               self.menu_option.size_items_column = self.menu_option.max_page_size / self.menu_option.max_columns;
               self.menu_option.init_position_x = 90;
               self.menu_option.init_position_y = -88;
               self.menu_option.padding_x = 212.0f;
               self.menu_option.padding_y = 12;
               self.menu_option.image_size = 200.0f;
         }
         break;

      case MT_PLANE_TEXT:
         {
               self.menu_option.max_page_size = 10;
               self.menu_option.max_columns = 1;
               self.menu_option.size_items_column = self.menu_option.max_page_size / self.menu_option.max_columns;
               self.menu_option.init_position_x = 20;
               self.menu_option.init_position_y = 5;
               self.menu_option.padding_x = 320;
               self.menu_option.padding_y = 12;
               self.menu_option.image_size = 64.0f;
         }
         break;
      
      default:
         {
               self.menu_option.max_page_size = 10;
               self.menu_option.max_columns = 2;
               self.menu_option.size_items_column = self.menu_option.max_page_size / self.menu_option.max_columns;
               self.menu_option.init_position_x = 20;
               self.menu_option.init_position_y = 5;
               self.menu_option.padding_x = 320;
               self.menu_option.padding_y = 12;
               self.menu_option.image_size = 64.0f;
         }
         break;
   }
}

static ImageDimensionStruct* GetImageDimension(const char *image_file)
{
   ImageDimensionStruct* image = NULL;
	char *image_type = strrchr(image_file, '.');

	if (strcasecmp(image_type, ".png") == 0 || strcasecmp(image_type, ".bpm") == 0) { 
      // || strcasecmp(image_type, ".jpg")) {

		file_t image_handle = fs_open(image_file, O_RDONLY);
	
		if(image_handle == FILEHND_INVALID)
			return NULL;

      image = (ImageDimensionStruct*)malloc(sizeof(ImageDimensionStruct));
      memset(image, 0, sizeof(ImageDimensionStruct));

		if (strcasecmp(image_type, ".png") == 0)	{
			fs_seek(image_handle, 16, SEEK_SET);
			fs_read(image_handle, (char *)&image->width, sizeof(uint32));

			fs_seek(image_handle, 20, SEEK_SET);
			fs_read(image_handle, (char *)&image->height, sizeof(uint32));
		}
      // else if (strcasecmp(image_type, ".jpg") == 0)	{
		// 	fs_seek(image_handle, 16, SEEK_SET);
		// 	fs_read(image_handle, (char *)&image->width, sizeof(uint32));

		// 	fs_seek(image_handle, 20, SEEK_SET);
		// 	fs_read(image_handle, (char *)&image->height, sizeof(uint32));
		// }
		else if (strcasecmp(image_type, ".bmp") == 0) {
			fs_seek(image_handle, 18, SEEK_SET);
			fs_read(image_handle, (char *)&image->width, sizeof(uint32));

			fs_seek(image_handle, 22, SEEK_SET);
			fs_read(image_handle, (char *)&image->height, sizeof(uint32));
		}

		image->width = ntohl(image->width);
		image->height = ntohl(image->height);

		fs_close(image_handle);
	}

	return image;
}

static void FreeGames()
{
#ifndef IN_CACHE_GAMES
   FreeGamesForce();
#endif
}

static void FreeGamesForce()
{
   if (self.games_array_count > 0) {
      // for(int freeCount = 0; freeCount < games_array_count; freeCount++) {
      //     free((*games_array + freeCount));
      // }
      free(self.games_array);
      
      self.games_array = NULL;
      self.games_array_count = 0;
    }
}

static void SetCursor()
{
   static Vector selector_translate = {0, 0, ML_CURSOR, 1};
                
   if (self.item_selector != NULL) {
      TSU_MenuSubRemoveBanner(self.dsmenu_ptr, self.item_selector);
      self.item_selector = NULL;
   }

   if (self.item_selector_animation != NULL) {
      TSU_AnimationDestroy(self.item_selector_animation);
      self.item_selector_animation = NULL;
   }

   if (self.item_selector_texture != NULL) {
      TSU_TextureDestroy(self.item_selector_texture);
      self.item_selector_texture = NULL;
   }

   if (self.item_selector == NULL) {      
      static char selector_path[NAME_MAX];
      memset(selector_path, 0, sizeof(selector_path));
      snprintf(selector_path, sizeof(selector_path), "%s/%s", self.default_dir, "apps/games/images/item_selector.png");

      if (self.menu_type == MT_IMAGE_TEXT_64_5X2) {
         self.item_selector_texture = TSU_TextureCreateFromFile(selector_path, true, false);
         self.item_selector = TSU_BannerCreate(PVR_LIST_TR_POLY, self.item_selector_texture);

         Color color = {1, 1.0f, 1.0f, 0.0f};
         TSU_DrawableSetTint((Drawable *)self.item_selector, &color);
         TSU_DrawableSetAlpha((Drawable *)self.item_selector, 0.7f);
      }
      else {
         self.item_selector_texture = TSU_TextureCreateFromFile(selector_path, true, false);
         self.item_selector = TSU_BannerCreate(PVR_LIST_TR_POLY, self.item_selector_texture);
         Color color = {1, 1.0f, 1.0f, 0.1f};
         TSU_DrawableSetTint((Drawable *)self.item_selector, &color);
         TSU_DrawableSetAlpha((Drawable *)self.item_selector, 0.7f);
      }

      if (selector_translate.x && selector_translate.y > 0) {
         TSU_DrawableSetTranslate((Drawable *)self.item_selector, &selector_translate);
      }

      TSU_MenuSubAddBanner(self.dsmenu_ptr, self.item_selector);
   }
   
   static int init_position_x;
   static int init_position_y;

   if (self.menu_type == MT_IMAGE_TEXT_64_5X2) {

      TSU_BannerSetSize(self.item_selector, 310, self.menu_option.image_size + 8);
      init_position_x = 320 / self.menu_option.max_columns - 5;
      init_position_y = 84 / self.menu_option.size_items_column + 8;
      selector_translate.z = 1;
   }
   else {
      TSU_BannerSetSize(self.item_selector, self.menu_option.image_size + 14, self.menu_option.image_size + 14);
      init_position_x = self.menu_option.init_position_x + 20;
      init_position_y = self.menu_option.init_position_y + 20;
      selector_translate.z = 1;
   }

   selector_translate.x = self.menu_cursel / self.menu_option.size_items_column * self.menu_option.padding_x + init_position_x;
   selector_translate.y = ((self.menu_cursel + 1) - self.menu_option.size_items_column * (self.menu_cursel / self.menu_option.size_items_column)) * (self.menu_option.padding_y + self.menu_option.image_size) + init_position_y;

   self.item_selector_animation = (Animation *)TSU_LogXYMoverCreate(selector_translate.x, selector_translate.y);
   TSU_DrawableAnimAdd((Drawable *)self.item_selector, (Animation *)self.item_selector_animation);
}

static bool IsValidImage(const char *image_file)
{
   bool isValid = false;

   ImageDimensionStruct* image = GetImageDimension(image_file);
   if (image != NULL)
   {
      isValid = (image->height == 64 && image->width == 64) 
                  || (image->height == 128 && image->width == 128)
                  || (image->height == 256 && image->width == 256);

      free(image);
   }

   return isValid;
}

static int AppCompareGames(const void *a, const void *b)
{
    const GameItemStruct *left  = (const GameItemStruct *)a;
    const GameItemStruct *right = (const GameItemStruct *)b;
    int cmp = 0;
    
    if (left->folder[0] != '\0' && right->folder[0] != '\0') {
        cmp = strcmp(left->folder, right->folder);
    }
    else if (left->folder[0] != '\0') {
        cmp = strcmp(left->folder, right->game);
    }
    else if (right->folder[0] != '\0') {
        cmp = strcmp(left->game, right->folder);
    }
    else {
        cmp = strcmp(left->game, right->game);
    }

	return cmp > 0 ? cmp : -1;
}

static int TotalPages()
{
   int fileCount = 0;
   char game_path[NAME_MAX];
   char *file_type = NULL;
   file_t fdg;
   dirent_t *entg = NULL;

   file_t fd = fs_open(self.games_path, O_RDONLY | O_DIR);
   dirent_t *ent;

   if(fd == FILEHND_INVALID)
      return 0;   

   while((ent = fs_readdir(fd)) != NULL) {
      if(ent->name[0] == '.') continue;

      file_type = strrchr(ent->name, '.');
      
      if (strcasecmp(file_type, ".cdi") == 0 || strcasecmp(file_type, ".iso") == 0) {
         fileCount++;
      }
      else {
         memset(game_path, '\0', sizeof(game_path));
         snprintf(game_path, sizeof(game_path), "%s/%s", self.games_path, ent->name);

         fdg = fs_open(game_path, O_RDONLY | O_DIR);
         
         if(fdg == FILEHND_INVALID)
               continue;

         while ((entg = fs_readdir(fdg)) != NULL) {
               if (entg->name[0] == '.') continue;
               
               file_type = strrchr(entg->name, '.');

               if (strcasecmp(file_type, ".gdi") == 0) {
                  fileCount++;
                  break;
               }
               else if (strcasecmp(file_type, ".cdi") == 0) {
                  fileCount++;
                  break;
               }
               else if (strcasecmp(file_type, ".iso") == 0 && !(strncasecmp(entg->name, "track", strlen("track")) == 0)) {
                  fileCount++;
                  break;
               }
         }
         fs_close(fdg);
      }
    }
    fs_close(fd);
    file_type = NULL;

    return (fileCount > 0 ? ceil((float)fileCount / (float)self.menu_option.max_page_size) : 0);
}

static bool RetrieveGames()
{
#ifdef IN_CACHE_GAMES
   if (self.games_array_count > 0) {
      return true;
   }
#endif

   FreeGames();
   
   char game_path[NAME_MAX];
   // char game_cover_path[NAME_MAX];

   file_t fd = fs_open(self.games_path, O_RDONLY | O_DIR);    
   if(fd == FILEHND_INVALID)
      return false;

   dirent_t *ent = NULL;
   file_t fdg;
   dirent_t *entg = NULL;
   char folder_game[MAX_SIZE_GAME_NAME];
   char game[MAX_SIZE_GAME_NAME];
   char *file_type = NULL;
   bool with_folder = false;
   
   while((ent = fs_readdir(fd)) != NULL) {
      
      if(ent->name[0] == '.') continue;
      
      with_folder = false;
      file_type = strrchr(ent->name, '.');
      memset(game, '\0', sizeof(game));
      
      if (strcasecmp(file_type, ".cdi") == 0 || (strcasecmp(file_type, ".iso") == 0  
                                                && !(strncasecmp(entg->name, "track", strlen("track")) == 0))) {
         strcpy(game, ent->name);
      }
      else {
         memset(game_path, '\0', sizeof(game_path));
         snprintf(game_path, sizeof(game_path), "%s/%s", self.games_path, ent->name);

         fdg = fs_open(game_path, O_RDONLY | O_DIR);
         
         if(fdg == FILEHND_INVALID)
            continue;

         with_folder = true;
         while ((entg = fs_readdir(fdg)) != NULL) {
            if (entg->name[0] == '.') continue;
            
            file_type = strrchr(entg->name, '.');

            if (strcasecmp(file_type, ".gdi") == 0) {
               strcpy(game, entg->name);
               break;
            }
            else if (strcasecmp(file_type, ".cdi") == 0) {
               strcpy(game, entg->name);
               break;
            }
            else if (strcasecmp(file_type, ".iso") == 0 && !(strncasecmp(entg->name, "track", strlen("track")) == 0)) {
               strcpy(game, entg->name);
               break;
            }
         }
         fs_close(fdg);
         entg = NULL;
      }

      if (game[0] != '\0') {

         for(char* c=game; *c=toupper(*c); ++c) {
            if (*c == 'a') *c = 'A'; //Maniac Vera: BUG toupper in the letter a, it does not convert it
         }

         memset(folder_game, 0, sizeof(folder_game));
         strncpy(folder_game, ent->name, strlen(ent->name));

         for(char* c=folder_game; *c=toupper(*c); ++c) {
            if (*c == 'a') *c = 'A'; //Maniac Vera: BUG toupper in the letter a, it does not convert it
         }

         self.games_array_count++;
         if (self.games_array == NULL) {
            self.games_array = (GameItemStruct*)malloc(sizeof(GameItemStruct));
         }
         else {
            self.games_array = (GameItemStruct*)realloc(self.games_array, self.games_array_count * sizeof(GameItemStruct));
         }

         memset(&self.games_array[self.games_array_count-1], 0, sizeof(GameItemStruct));
         strncpy(self.games_array[self.games_array_count-1].game, game, strlen(game));

         if (with_folder) {
            strncpy(self.games_array[self.games_array_count-1].folder, folder_game, strlen(folder_game));
         }
         self.games_array[self.games_array_count-1].with_folder = with_folder;
         self.games_array[self.games_array_count-1].exists_cover = SC_WITHOUT_SEARCHING;
      }
   }
   fs_close(fd);
   ent = NULL;
   file_type = NULL;
	
	if (self.games_array_count > 0) {
		qsort(self.games_array, self.games_array_count, sizeof(GameItemStruct), AppCompareGames);
      return true;
	}
   else {
      return false;
   }
}

static bool LoadPage(bool force)
{
   bool loaded = false;
   char game_cover_path[NAME_MAX];   
   char name[MAX_SIZE_GAME_NAME];
   char game[MAX_SIZE_GAME_NAME];
   char game_without_extension[MAX_SIZE_GAME_NAME];
   char name_truncated[21];
   int fileCount = 0;

   if (self.pages == -1) {
      self.pages = TotalPages();
   }
   else {
      if (force) {
         self.pages = (self.games_array_count > 0 ? ceil((float)self.games_array_count / (float)self.menu_option.max_page_size) : 0);
      }
   }
   
   if (self.pages > 0) {
      
      if (self.current_page < 1) {
         // self.current_page = 1;
         self.current_page = self.pages;
      }
      else if (self.current_page > self.pages) {
         // self.current_page = self.pages;
         self.current_page = 1;
      }

      if (self.current_page != self.previous_page || force) {
         self.previous_page = self.current_page;

         for(int i = 0; i < MAX_SIZE_ITEMS; i++) {
            if (self.img_button_animation[i] != NULL) {
               TSU_AnimationDestroy(self.img_button_animation[i]);
            }
            
            if (self.img_button[i] != NULL) {
               TSU_MenuSubRemoveItemMenu(self.dsmenu_ptr, self.img_button[i]);
            }

            self.img_button[i] = NULL;
            self.img_button_animation[i] = NULL;
         }

         if (RetrieveGames()) {               
               int column = 0;
               int page = 0;
               self.game_count = 0;
               Vector vectorTranslate = {0, 480, ML_ITEM, 1};

               for(int icount = 0; icount < self.games_array_count; icount++) {
                  fileCount++;
                  page = ceil((float)fileCount / (float)self.menu_option.max_page_size);

                  if (page < self.current_page)
                     continue;
                  else if (page > self.current_page)
                     break;

                  self.game_count++;

                  column = floor((float)(self.game_count-1) / (float)self.menu_option.size_items_column);

                  if (self.games_array[icount].exists_cover == SC_WITHOUT_SEARCHING) {
                     memset(game_without_extension, 0, sizeof(game_without_extension));
                     strncpy(game_without_extension, self.games_array[icount].game, strlen(self.games_array[icount].game)-4);

                     // memset(game_cover_path, 0, sizeof(game_cover_path));
                     // snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.jpg", self.covers_path, game_without_extension);
                     // if (IsValidImage(game_cover_path)) {
                     //    self.games_array[icount].exists_cover = SC_EXISTS;
                     //    self.games_array[icount].cover_type = IT_JPG;
                     // }
                              
                     memset(game_cover_path, 0, sizeof(game_cover_path));
                     snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.png", self.covers_path, game_without_extension);
                     if (IsValidImage(game_cover_path)) {
                        self.games_array[icount].exists_cover = SC_EXISTS;
                        self.games_array[icount].cover_type = IT_PNG;
                     }
                     
                     // DEFAULT IMAGE PNG
                     if (self.games_array[icount].exists_cover == SC_WITHOUT_SEARCHING) {
                        self.games_array[icount].exists_cover = SC_NOT_EXIST;
                        self.games_array[icount].cover_type = IT_PNG;
                     }
                  }

                  if (self.games_array[icount].exists_cover == SC_EXISTS) {
                     memset(game, 0, sizeof(game));
                     strncpy(game, self.games_array[icount].game, strlen(self.games_array[icount].game)-4);

                     if (self.games_array[icount].cover_type == IT_JPG) {
                        snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.jpg", self.covers_path, game);
                     }
                     else if (self.games_array[icount].cover_type == IT_PNG) {
                        snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s.png", self.covers_path, game);
                     }
                     else {
                        // NEVER HERE!
                     }
                  }
                  else {
                     snprintf(game_cover_path, sizeof(game_cover_path), "%s/%s", self.default_dir, "apps/games/images/gd.png");
                  }
                  
                  memset(name, 0 , sizeof(name));
                  if(self.games_array[icount].folder[0] != '\0') {
                     strncpy(name, self.games_array[icount].folder, strlen(self.games_array[icount].folder));
                  }
                  else {
                     strncpy(name, self.games_array[icount].game, strlen(self.games_array[icount].game)-4);
                  }                  

                  if (self.menu_type == MT_IMAGE_TEXT_64_5X2) {

                     memset(name_truncated, 0, sizeof(name_truncated));
                     if (strlen(name) > sizeof(name_truncated) - 1) {
                        strncpy(name_truncated, name, sizeof(name_truncated) - 1);
                     }
                     else {
                        strcpy(name_truncated, name);
                     }

                     self.img_button[self.game_count-1] = TSU_ItemMenuCreate(game_cover_path, self.menu_option.image_size, self.menu_option.image_size, name_truncated, self.menu_font, 16);
                  }
                  else if (self.menu_type == MT_PLANE_TEXT) {
                  }
                  else {
                     self.img_button[self.game_count-1] = TSU_ItemMenuCreateImage(game_cover_path, self.menu_option.image_size, self.menu_option.image_size);
                  }

                  ItemMenu *item_menu = self.img_button[self.game_count-1];

                  TSU_ItemMenuSetItemIndex(item_menu, icount);
                  if (self.games_array[icount].with_folder) {
                     TSU_ItemMenuSetItemValue(item_menu, name);
                  }
                  else {
                     TSU_ItemMenuSetItemValue(item_menu, name);
                  }
                  
                  TSU_ItemMenuSetTranslate(item_menu, &vectorTranslate);
                  
                  self.img_button_animation[self.game_count-1] = (Animation *)TSU_LogXYMoverCreate(self.menu_option.init_position_x + (column * self.menu_option.padding_x), 
                           (self.menu_option.image_size + self.menu_option.padding_y) * (self.game_count - self.menu_option.size_items_column * column) + self.menu_option.init_position_y);
                           
                  TSU_ItemMenuAnimAdd(item_menu, self.img_button_animation[self.game_count-1]);

                  if (self.game_count == 1) {
                     TSU_ItemMenuSetSelected(item_menu, true);
                     SetTitle(name);
                     SetCursor();
                  }
                  else {
                     TSU_ItemMenuSetSelected(item_menu, false);
                  }
                  
                  TSU_MenuSubAddItemMenu(self.dsmenu_ptr, item_menu);
               }

               loaded = true;
               FreeGames();
         }        
      }
   }
   else {
      memset(game_cover_path, 0, sizeof(game_cover_path));
      snprintf(game_cover_path, sizeof(game_cover_path), "You need put the games here:\n%s\n\nand images here:\n%s", self.games_path, self.covers_path);
      ds_printf(game_cover_path);
      SetTitle(game_cover_path);
   }

   return loaded;  
}

static void InitMenu()
{
   // // snd_stream_init();
   // // mp3_init();
   // // mp3_start("/rd/ide/DS/apps/games/music/music.mp3", 0);
   self.scene_ptr = TSU_MenuGetScene(self.dsmenu_ptr);
   memset(self.default_dir, 0, sizeof(self.default_dir));
   memset(self.covers_path, 0, sizeof(self.covers_path));
   memset(self.games_path, 0, sizeof(self.games_path));

   // getenv("PATH")
   if(DirExists("/ide/games")) {
      strcpy(self.default_dir, "/ide/DS");
      strcpy(self.games_path, "/ide/games");
      strcpy(self.covers_path, "/ide/DS/apps/games/covers");
   }
   else if(DirExists("/sd/games")) {
      strcpy(self.default_dir, "/sd/DS");
      strcpy(self.games_path, "/sd/games");
      strcpy(self.covers_path, "/sd/DS/apps/games/covers");
   }
   else if(!is_custom_bios()) {
      strcpy(self.default_dir, "/cd");

#ifdef DEBUG_MENU_GAMES
      strcpy(self.games_path, "/cd/apps/games/gdis");
      strcpy(self.covers_path, "/cd/apps/games/covers");
#else
      strcpy(self.games_path, "/cd/games");
      strcpy(self.covers_path, "/cd/apps/games/covers");
#endif 

   }

   char font_path[NAME_MAX];
   memset(font_path, 0, sizeof(font_path));
   snprintf(font_path, sizeof(font_path), "%s/%s", self.default_dir, "apps/games/fonts/default.txf");

   self.menu_font = TSU_FontCreate(font_path, PVR_LIST_TR_POLY);
   // TSU_FontCreateInternal(self.menu_font, font_path, PVR_LIST_TR_POLY);

   SetMenuType(MT_IMAGE_TEXT_64_5X2);
   self.exit = false;
   self.title = NULL;

   // Set a green background
   // TSU_MenuSetBg(self.dsmenu_ptr, 0.06f, 0.41f, 0.81f);   
   // TSU_MenuSetBg(self.dsmenu_ptr, 0.0f, 0.0f, 0.0f);
   
   // Offset our scene so 0,0,0 is the screen center with Z +10
   Vector vector = {0, 0, 10, 1};
   TSU_MenSetTranslate(self.dsmenu_ptr, &vector);

   self.current_page = 1;
   LoadPage(false);

   self.menu_cursel = 0;
}

static void StartExit()
{
   StopCDDATrack();

   float y = 1.0f;

   if (self.item_selector != NULL) {
      TSU_MenuSubRemoveBanner(self.dsmenu_ptr, self.item_selector);
      self.item_selector = NULL;
   }

   if (self.title != NULL) {
      TSU_MenuSubRemoveLabel(self.dsmenu_ptr, self.title);
      self.title = NULL;
   }

   for(int i = 0; i < MAX_SIZE_ITEMS; i++) {

      if (self.img_button[i] != NULL) {

         if (!self.exit_app && TSU_ItemMenuIsSelected(self.img_button[i])) {
               
               int game_index = TSU_ItemMenuGetItemIndex(self.img_button[i]);
               if (game_index >= 0) {                  
                  char *full_path_game = (char *)malloc(NAME_MAX);
                  memset(full_path_game, 0, sizeof(full_path_game));

                  if (self.games_array[game_index].with_folder) {                     
                     snprintf(full_path_game, NAME_MAX, "%s/%s/%s", self.games_path, self.games_array[game_index].folder, self.games_array[game_index].game);
                  }
                  else {
                     snprintf(full_path_game, NAME_MAX, "%s/%s", self.games_path, self.games_array[game_index].game);
                  }

                  strcpy(self.item_value_selected, full_path_game);
                  free(full_path_game);
               }

               if (TSU_ItemMenuHasTextAndImage(self.img_button[i])) {
                  self.exit_animation_list[i] = (Animation *)TSU_LogXYMoverCreate((640-(84+64))/2, (480-84)/2);
               }
               else {
                  self.exit_animation_list[i] = (Animation *)TSU_LogXYMoverCreate((640-84)/2, (480-84)/2);
               }

               self.exit_trigger_list[i] = (Trigger *)TSU_DeathCreate(NULL);
               TSU_TriggerAdd(self.exit_animation_list[i], self.exit_trigger_list[i]);
               TSU_DrawableAnimAdd((Drawable *)self.img_button[i], self.exit_animation_list[i]);
         }
         else {
               self.exit_animation_list[i] = (Animation *)TSU_ExpXYMoverCreate(0, y+.10, 0, 400);
               self.exit_trigger_list[i] = (Trigger *)TSU_DeathCreate(NULL);

               TSU_TriggerAdd(self.exit_animation_list[i], self.exit_trigger_list[i]);
               TSU_DrawableAnimAdd((Drawable *)self.img_button[i], self.exit_animation_list[i]);
         }
      }
   }
   TSU_MenuStartExit(self.dsmenu_ptr);
}

static void GamesApp_InputEvent(int type, int key)
{
   if(type != EvtKeypress)
      return;
      
   switch(key) {

      // X: CHANGE VISUALIZATION
      case KeyMiscX: {
               int real_cursel = (self.current_page - 1) * self.menu_option.max_page_size + self.menu_cursel + 1;            
               SetMenuType(self.menu_type >= 2 ? MT_IMAGE_TEXT_64_5X2 : self.menu_type + 1);
               self.current_page = ceil((float)real_cursel / (float)self.menu_option.max_page_size);

               if (LoadPage(true)) {
                  self.menu_cursel = real_cursel - (self.current_page - 1) *self. menu_option.max_page_size - 1;
               }
         }
         break;

      case KeyCancel: {
            // if ((new_time.tv_sec - old_time.tv_sec) > 1) {
            //    old_time.tv_sec = 0;
            //    new_time.tv_sec = 0;
            // }

            // old_time = new_time;
            // rtc_gettimeofday(&new_time);

            // if ((new_time.tv_sec - old_time.tv_sec) == 1) {
               self.exit_app = true;
               self.exit = true;
               StartExit();
            // }
         }
         break;

      // LEFT TRIGGER
      case KeyPgup: {
               self.current_page--;
               if (LoadPage(false)) {
                  self.menu_cursel = 0;
               }
         }
         break;

      // RIGHT TRIGGER
      case KeyPgdn: {                    
               self.current_page++;
               if (LoadPage(false)) {
                  self.menu_cursel = 0;
               }
         }
         break;
         
      case KeyUp: {
               self.menu_cursel--;

               if (self.menu_type == MT_IMAGE_TEXT_64_5X2) {
                  if(self.menu_cursel < 0) {
                     self.menu_cursel += self.game_count;
                  }
               }
               else {
                  if((self.menu_cursel + 1) % self.menu_option.size_items_column == 0) {
                    self. menu_cursel += self.menu_option.size_items_column;
                  }
                  
                  if(self.menu_cursel < 0) {
                     self.menu_cursel = 0;
                  }
               }
         }

         break;

      case KeyDown: {
               self.menu_cursel++;

               if (self.menu_type == MT_IMAGE_TEXT_64_5X2) {
                  if(self.menu_cursel >= self.game_count) {
                     self.menu_cursel -= self.game_count;
                  }
               }
               else {

                  if(self.menu_cursel % self.menu_option.size_items_column == 0) {
                     self.menu_cursel -= self.menu_option.size_items_column;
                  }

                  if(self.menu_cursel >= self.game_count) {
                     self.menu_cursel = self.game_count;
                  }
               }
         }
         
         break;

      case KeyLeft:
         if (self.game_count <= self.menu_option.size_items_column) {
            self.menu_cursel=0;
         }
         else {
            self.menu_cursel-=self.menu_option.size_items_column;

            if(self.menu_cursel < 0)
               self.menu_cursel += self.game_count;
         }

         break;

      case KeyRight:
         if (self.game_count <= self.menu_option.size_items_column) {
            self.menu_cursel=self.game_count-1;
         }
         else {
            self.menu_cursel+=self.menu_option.size_items_column;
            
            if(self.menu_cursel >= self.game_count)
               self.menu_cursel -= self.game_count;
         }         

         break;

      case KeySelect:
         StartExit(false);
         self.exit = true;
         break;
         
      default:
         printf("Unhandled Event Key\n");
         break;
   }

   if (!self.exit) {
      for(int i = 0; i < self.game_count; i++) {
         if(i == self.menu_cursel) {
            TSU_ItemMenuSetSelected(self.img_button[i], true);
            SetTitle(TSU_ItemMenuGetItemValue(self.img_button[i]));
            SetCursor();
         }
         else {
            TSU_ItemMenuSetSelected(self.img_button[i], false);
         }
      }
   }
}

static int CanUseTrueAsyncDMA(void)
{
	return (self.sector_size == 2048 && 
			(self.current_dev == APP_DEVICE_IDE || self.current_dev == APP_DEVICE_CD) &&
			(self.image_type == ISOFS_IMAGE_TYPE_ISO || self.image_type == ISOFS_IMAGE_TYPE_GDI));
}

// static int GetImageType(const char *filename)
// {
//    int result_image_type = -1;
//    if (filename != NULL && strlen(filename) > 4) {
//       char *image_type = strrchr(filename, '.');

//       if (strcasecmp(image_type, ".gdi") == 0) {
// 	      result_image_type = ISOFS_IMAGE_TYPE_GDI;
//       }
//       else if (strcasecmp(image_type, ".cdi") == 0) {
//          result_image_type = ISOFS_IMAGE_TYPE_CDI;
//       }
//       else if (strcasecmp(image_type, ".iso") == 0) {
//          result_image_type = ISOFS_IMAGE_TYPE_ISO;
//       }      
//    }

//    return result_image_type;
// }

static void SendMessageEvent(const char* message)
{
   ds_printf("DS_MESSAGE: %s\n", message);
}

void DefaultPreset()
{
	if(CanUseTrueAsyncDMA()) {
		self.isoldr->use_dma = 1;
      self.isoldr->emu_async = 0;
      self.addr = ISOLDR_DEFAULT_ADDR_MIN_GINSU;
   }
   else {
      self.isoldr->use_dma = 0;
      self.isoldr->emu_async = 8; // self.async[8]
      self.addr = ISOLDR_DEFAULT_ADDR_LOW;
   }
   
   strcpy(self.isoldr->fs_dev, "auto");
   self.isoldr->emu_cdda = CDDA_MODE_DISABLED;
	self.isoldr->use_irq = 0;
   self.isoldr->syscalls = 0;
   self.isoldr->emu_vmu = 0;
	self.isoldr->scr_hotkey = 0;
   // self.isoldr->exec.type = BIN_TYPE_AUTO; 
   self.isoldr->boot_mode = BOOT_MODE_DIRECT;
   
	// Enable CDDA if present
	if (self.item_value_selected[0] != '\0') {

		char track_file_path[NAME_MAX];
      char game_with_folder[NAME_MAX];

      memset(game_with_folder, 0, sizeof(game_with_folder));
      strcpy(game_with_folder, self.item_value_selected + strlen(self.games_path) + 1);

		size_t track_size = GetCDDATrackFilename(4, self.games_path, game_with_folder, track_file_path);

		if (track_size && track_size < 30 * 1024 * 1024) {
			track_size = GetCDDATrackFilename(6, self.games_path, game_with_folder, track_file_path);
		}

		if (track_size > 0) {
         self.isoldr->use_irq = 1;
         self.isoldr->emu_cdda = 1;    
		}
	}
}

static void GetMD5Hash(const char *file_mount_point) {
	file_t fd;
	fd = fs_iso_first_file(file_mount_point);
	
	if(fd != FILEHND_INVALID) {

		if(fs_ioctl(fd, ISOFS_IOCTL_GET_BOOT_SECTOR_DATA, (int) self.boot_sector) < 0) {
			memset(self.md5, 0, sizeof(self.md5));
			memset(self.boot_sector, 0, sizeof(self.boot_sector));
		} else {
			kos_md5(self.boot_sector, sizeof(self.boot_sector), self.md5);
		}
		
		/* Also get image type and sector size */
		if(fs_ioctl(fd, ISOFS_IOCTL_GET_IMAGE_TYPE, (int) &self.image_type) < 0) {
			ds_printf("%s: Can't get image type\n", lib_get_name());
		}
		
		if(fs_ioctl(fd, ISOFS_IOCTL_GET_DATA_TRACK_SECTOR_SIZE, (int) &self.sector_size) < 0) {
			ds_printf("%s: Can't get sector size\n", lib_get_name());
		}
		
		fs_close(fd);
	}
}

static int LoadPreset()
{
   char *preset_file_name = NULL;
   if (fs_iso_mount("/iso", self.item_value_selected) == 0) {
      GetMD5Hash("/iso");
      fs_iso_unmount("/iso");
      preset_file_name = MakePresetFilename(self.default_dir, self.md5);
      ds_printf("PresetFileName: %s", preset_file_name);

      if (FileSize(preset_file_name) < 5) {
         preset_file_name = NULL;
      }
   }
   // else {
   //    ds_printf("could not mound iso: %s\n", self.item_value_selected);
   //    return -1;
   // }
   
   self.current_dev = GetDeviceType(self.default_dir);

   int use_dma = 0, emu_async = 16, use_irq = 0;
   int fastboot = 0, low = 0, emu_vmu = 0, scr_hotkey = 0;
   int boot_mode = BOOT_MODE_DIRECT;
   int bin_type = BIN_TYPE_AUTO;
   // uint32 heap = HEAP_MODE_AUTO;
   bool default_preset_loaded = false;
   char title[32] = "";
   char device[8] = "";
   char cdda[12] = "";
   char memory[12] = "0x8c000100";
   char heap_memory[12] = "";
   char bin_file[12] = "";
   char patch_a[2][10];
   char patch_v[2][10];
   // int i, len;
   // char *name;
   memset(patch_a, 0, 2 * 10);
   memset(patch_v, 0, 2 * 10);

   isoldr_conf options[] = {
      { "dma",      CONF_INT,   (void *) &use_dma    },
      { "cdda",     CONF_STR,   (void *) cdda        },
      { "irq",      CONF_INT,   (void *) &use_irq    },
      { "low",      CONF_INT,   (void *) &low        },
      { "vmu",      CONF_INT,   (void *) &emu_vmu    },
      { "scrhotkey",CONF_INT,   (void *) &scr_hotkey },
      { "heap",     CONF_STR,   (void *) &heap_memory},
      { "memory",   CONF_STR,   (void *) memory      },
      { "async",    CONF_INT,   (void *) &emu_async  },
      { "mode",     CONF_INT,   (void *) &boot_mode  },
      { "type",     CONF_INT,   (void *) &bin_type   },
      { "file",     CONF_STR,   (void *) bin_file    },
      { "title",    CONF_STR,   (void *) title       },
      { "device",   CONF_STR,   (void *) device      },
      { "fastboot", CONF_INT,   (void *) &fastboot   },
      { "pa1",      CONF_STR,   (void *) patch_a[0]  },
      { "pv1",      CONF_STR,   (void *) patch_v[0]  },
      { "pa2",      CONF_STR,   (void *) patch_a[1]  },
      { "pv2",      CONF_STR,   (void *) patch_v[1]  },
      { NULL,       CONF_END,   NULL				   }
   };


   if ((self.isoldr = isoldr_get_info(self.item_value_selected, 0)) != NULL) {

      if (preset_file_name == NULL || conf_parse(options, preset_file_name) == -1) {
         ds_printf("DS_ERROR: Can't parse preset\n");
         DefaultPreset();
         default_preset_loaded = true;
      }

      if (!default_preset_loaded) {
         // uint32 addr = ISOLDR_DEFAULT_ADDR_LOW;
         self.isoldr->use_dma = use_dma;
         self.isoldr->emu_async = emu_async;
         self.isoldr->emu_cdda = strtoul(cdda, NULL, 16);
         self.isoldr->use_irq = use_irq;
         // self.isoldr->emu_vmu = emu_vmu;
         self.isoldr->scr_hotkey = scr_hotkey;  
   
         // char heap_memory_text[24];
         // memset(heap_memory_text, 0, sizeof(heap_memory_text));

         if (strtoul(heap_memory, NULL, 10) <= HEAP_MODE_MAPLE) {
            self.isoldr->heap = strtoul(heap_memory, NULL, 10);
         }
         else {
            self.isoldr->heap = strtoul(heap_memory, NULL, 16);
         }

         self.isoldr->boot_mode = boot_mode;
         // self.isoldr->image_type = 0;
         self.isoldr->fast_boot = fastboot;

         if (strlen(device) > 0) {
            if(strncmp(device, "auto", 4) != 0) {
               strcpy(self.isoldr->fs_dev, device);
            }
            else {
               strcpy(self.isoldr->fs_dev, "auto");
            }
         }
         else {
            strcpy(self.isoldr->fs_dev, "auto");
         }

         if (bin_type != BIN_TYPE_AUTO) {
            self.isoldr->exec.type = bin_type;
         }

         if (low) {
            self.isoldr->syscalls = 1;
         }

         self.addr = strtoul(memory, NULL, 16);
      }

      if(strncmp(self.isoldr->fs_dev, "auto", 4) == 0) {
         if(!strncasecmp(self.default_dir, "/cd", 3)) {
            strcpy(self.isoldr->fs_dev, "cd");
         } else if(!strncasecmp(self.default_dir, "/sd",  3)) {
            strcpy(self.isoldr->fs_dev, "sd");
         } else if(!strncasecmp(self.default_dir, "/ide", 4)) {
            strcpy(self.isoldr->fs_dev, "ide");
         }
      }

      // ds_printf("fs_dev: %s, memory: %u, use_dma: %u, emu_async: %u\nemu_cdda: %u, use_irq: %u, emu_vmu: %u, scr_hotkey: %u\nheap: %u, boot_mode: %u, fast_boot: %u\nexec.type: %u", 
      //    self.isoldr->fs_dev, 
      //    self.addr, //memory, 
      //    self.isoldr->use_dma,
      //    self.isoldr->emu_async,
      //    self.isoldr->emu_cdda,
      //    self.isoldr->use_irq,
      //    self.isoldr->emu_vmu,
      //    self.isoldr->scr_hotkey,
      //    self.isoldr->heap,
      //    self.isoldr->boot_mode,
      //    self.isoldr->fast_boot,
      //    self.isoldr->exec.type);

      // if(GUI_WidgetGetState(self.alt_boot)) {
      //    isoldr_set_boot_file(self.isoldr, filepath, ALT_BOOT_FILE);
      // }

      return 1;
   }
   else {
      return 0;
   }
}

void FreeAppData()
{
   for(int i = 0; i < MAX_SIZE_ITEMS; i++) {
      if (self.img_button_animation[i] != NULL) {
         TSU_AnimationDestroy(self.img_button_animation[i]);
      }
      
      if (self.img_button[i] != NULL) {
         TSU_MenuSubRemoveItemMenu(self.dsmenu_ptr, self.img_button[i]);
      }

      if (self.exit_animation_list[i] != NULL) {
         TSU_AnimationDestroy(self.exit_animation_list[i]);
      }

      if (self.exit_trigger_list[i] != NULL) {
         TSU_TriggerDestroy(self.exit_trigger_list[i]);
      }

      self.img_button[i] = NULL;
      self.img_button_animation[i] = NULL;
      self.exit_animation_list[i] = NULL;
      self.exit_trigger_list[i] = NULL;
   }

   if (self.item_selector != NULL) {
      TSU_MenuSubRemoveBanner(self.dsmenu_ptr, self.item_selector);
      self.item_selector = NULL;
   }

   if (self.title != NULL) {
      TSU_MenuSubRemoveLabel(self.dsmenu_ptr, self.title);
      self.title = NULL;
   }
   
   // TSU_BannerDestroyAll(self.item_selector);
   // TSU_LabelDestroy(self.title);   
   TSU_AnimationDestroy(self.item_selector_animation);
   TSU_TextureDestroy(self.item_selector_texture);
   TSU_FontDestroy(self.menu_font);
   TSU_MenuDestroy(self.dsmenu_ptr);
   FreeGamesForce();

   self.item_selector = NULL;
   self.item_selector_animation = NULL;
   self.item_selector_texture = NULL;
   self.dsmenu_ptr = NULL;
   self.scene_ptr = NULL;
   self.title = NULL;
}

bool RunGame() {
   bool is_running = false;
   
   if (self.item_value_selected[0] != '\0') {
      ds_printf("DS_GAMES: Run: %s", self.item_value_selected);

      if (LoadPreset() == 1) {
         ds_printf("LoadPresset: %s", "OK");        
         FreeAppData();

         isoldr_exec(self.isoldr, self.addr);
         is_running = true;
      }
   }

   return is_running;
}

static void MenuExitHelper(void *params)
{
   // NEED IT TO SLEEP BECAUSE MARKS ERROR IN OPEN FUNCTION
   thd_sleep(1000);
   
   if (self.dsmenu_ptr != NULL) {      

      if (!self.exit_app) {
         
         if (RunGame()) {
            InitVideoThread();
            SDL_DC_EmulateMouse(SDL_TRUE);
            SDL_DC_EmulateKeyboard(SDL_TRUE);
            EnableScreen();
            GUI_Enable();
            ShutdownDS();
         }
         else {
            FreeAppData();
            InitVideoThread();
            SDL_DC_EmulateMouse(SDL_TRUE);
            SDL_DC_EmulateKeyboard(SDL_TRUE);
            EnableScreen();
            GUI_Enable();

            GamesApp_Exit(NULL);
         }
      }
      else {
         FreeAppData();
         InitVideoThread();
         SDL_DC_EmulateMouse(SDL_TRUE);
         SDL_DC_EmulateKeyboard(SDL_TRUE);
         EnableScreen();
         GUI_Enable();

         GamesApp_Exit(NULL);
      }
   }
   
   return;
}

void GamesApp_ExitMenuEvent() {   
   exit_menu_thread = thd_create(0, &MenuExitHelper, NULL);   
}

void GamesApp_Init(App_t *app)
{
   if (exit_menu_thread) {
      thd_destroy(exit_menu_thread);
   }
   exit_menu_thread = NULL;

   if(app->args != NULL) {
      self.have_args = true;
   }
   else {
      self.have_args = false;
   }

   // TSU_SendMessageCallback(SendMessageEvent);
   self.isoldr = NULL;
   self.sector_size = 2048;
   self.exit = false;
   self.exit_app = false;
   self.pages = -1;
   self.current_page = 0;
   self.previous_page = 0;
   self.game_count = 0;
   self.menu_cursel = 0;
   self.games_array_count = 0;
   self.games_array = NULL;
   self.item_selector = NULL;
   self.menu_font = NULL;
   self.item_selector_animation = NULL;
   self.title = NULL;    
   memset(&self.menu_option, 0, sizeof(self.menu_option));
   self.menu_type = MT_IMAGE_TEXT_64_5X2;

   memset(self.item_value_selected, 0, sizeof(self.item_value_selected));

   for(int i = 0; i < MAX_SIZE_ITEMS; i++) {
      self.img_button[i] = NULL;
      self.img_button_animation[i] = NULL;
      self.exit_animation_list[i] = NULL;
      self.exit_trigger_list[i] = NULL;
   }

   if ((self.dsmenu_ptr = TSU_MenuCreateWithExit(GamesApp_InputEvent, GamesApp_ExitMenuEvent)) != NULL) {
      InitMenu();
   }   
}

void GamesApp_Open(App_t *app)
{
   (void)app;

   if (self.dsmenu_ptr != NULL) {
      int mx = 0;
      int my = 0;
      SDL_GetMouseState(&mx, &my);
      SDL_DC_EmulateMouse(SDL_FALSE);
      SDL_DC_EmulateKeyboard(SDL_FALSE);

      DisableScreen();
      GUI_Disable();
      ShutdownVideoThread();
      TSU_MenuDoMenu(self.dsmenu_ptr);
   }
}

void GamesApp_Shutdown(App_t *app)
{
   (void)app;

   if(self.isoldr) {
		free(self.isoldr);      
	}
}

void GamesApp_Exit(GUI_Widget *widget)
{
   (void)widget;

   App_t *app = NULL;
   if(self.have_args == true) {
		
		app = GetAppByName("File Manager");
		
		if(!app || !(app->state & APP_STATE_LOADED)) {
			app = NULL;
		}
	}
	if(!app) {
		app = GetAppByName("Main");
	}
   
	OpenApp(app, NULL);
}